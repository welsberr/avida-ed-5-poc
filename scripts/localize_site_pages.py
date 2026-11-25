#!/usr/bin/env python3
"""Create reviewable static-page translation drafts with a local Ollama model.

The generated HTML is committed for the Pages build; regeneration requires
Beautiful Soup and an explicitly supplied TalkOrigins compendium. A separate
stdlib-only assembler checks the source fingerprints before deployment.
"""
import argparse
from collections import Counter
import datetime
import hashlib
import html
import json
import re
import urllib.request
from pathlib import Path

from bs4 import BeautifulSoup

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    'index.html': ROOT / 'index.html',
    **{name: ROOT / 'docs' / name for name in (
        'port-status.html', 'codex-collaboration.html', 'porting-learning-roadmap.html',
        'roadmap.html', 'visual-audit-2026-09-30.html')},
}
OUTPUT = ROOT / 'docs/site-locales'
LANGUAGES = {'es': 'Spanish', 'fr': 'French', 'de': 'German', 'pt': 'Brazilian Portuguese'}
ROADMAP_IDS = tuple(m['id'] for m in json.loads((ROOT / 'docs/roadmap/progress.json').read_text(encoding='utf-8'))['milestones'] if '-' in m['id'])
IDENTIFIERS = re.compile(r'\b(?:' + '|'.join(re.escape(item) for item in ROADMAP_IDS)
                         + r')\b|\b(?=[a-f0-9]{7,40}\b)(?=[a-f0-9]*[0-9])(?=[a-f0-9]*[a-f])[a-f0-9]+\b', re.I)
TOKENS = re.compile(r'⟦[TPN]\d+⟧')
PROTECT = re.compile(
    r'<code\b[^>]*>.*?</code\s*>|<[^>]+>|' + IDENTIFIERS.pattern + r'|'
    r'Avida-ED(?:\s*[45])?|Avida\s*5|Plotly\.js|Emscripten|Empirical|'
    r'OpenAI|Codex|TalkOrigins|BEACON|GitHub|WebAssembly|'
    r'(?<![\w])\d+(?:[.,:/-]\d+)*(?:\s*[×%]\s*\d*)?', re.S | re.I)
SELECTORS = ('h1,h2,h3,h4,h5,h6,p,li,th,td,dt,dd,figcaption,nav,'
             '.eyebrow,footer .wrap,a.launch,a.skip-link,.summary-counts span')


def protect(source):
    substitutions = []
    def replace(match):
        kind = 'T' if match.group(0).startswith('<') else 'N' if match.group(0)[0].isdigit() else 'P'
        token = f'⟦{kind}{len(substitutions)}⟧'
        substitutions.append((token, match.group(0)))
        return token
    return html.unescape(PROTECT.sub(replace, source)), substitutions


def valid_tokens(source, value):
    expected, actual = TOKENS.findall(source), TOKENS.findall(value)
    tags = [token for token in expected if token.startswith('⟦T')]
    for first, second in zip(tags, tags[1:]):
        if first not in value or second not in value:
            continue
        original_between = source.split(first, 1)[1].split(second, 1)[0]
        translated_between = value.split(first, 1)[1].split(second, 1)[0]
        if original_between.strip() and not translated_between.strip():
            return False
    source_sentences = len(re.findall(r'[.!?](?=\s|$)', source))
    value_sentences = len(re.findall(r'[.!?](?=\s|$)', value))
    visible = TOKENS.sub('', source)
    normalize = lambda text: re.sub(r'[^\w]+', ' ', text.casefold()).strip()
    identifiers = IDENTIFIERS.findall(source)
    unchanged_english = (normalize(source) == normalize(value) and len(visible.strip()) > 16
                         and re.search(r'\b(?:the|and|with|for|from|to|of|not|that|this|while|each|when|where|which|have|an|or|no|how|does|what|these|by|into|are|is|as)\b', visible, re.I))
    return (Counter(expected) == Counter(actual)
            and [token for token in expected if token.startswith('⟦T')] == [token for token in actual if token.startswith('⟦T')]
            and [token for token in expected if token.startswith('⟦N')] == [token for token in actual if token.startswith('⟦N')]
            and Counter(re.findall(r'\d+', TOKENS.sub('', source))) == Counter(re.findall(r'\d+', TOKENS.sub('', value)))
            and all(value.count(identifier) == source.count(identifier) for identifier in set(identifiers))
            and (source_sentences < 2 or value_sentences >= source_sentences)
            and not unchanged_english)


def unit(source):
    protected, substitutions = protect(source)
    if not re.search(r'[A-Za-z]', TOKENS.sub('', protected)):
        return None, substitutions
    return protected, substitutions


def targets(soup):
    for block in soup.select(SELECTORS):
        yield ('html', block, None, block.decode_contents())
    if soup.title:
        yield ('text', soup.title, None, soup.title.get_text())
    for tag in soup.select('meta[name="description"]'):
        if tag.has_attr('content'):
            yield ('attribute', tag, 'content', tag['content'])
    for tag in soup.select('[aria-label], img[alt]'):
        for name in ('aria-label', 'alt'):
            if tag.has_attr(name) and tag[name].strip():
                yield ('attribute', tag, name, tag[name])


def load_sources():
    unique = {}
    for path in SOURCES.values():
        soup = BeautifulSoup(path.read_text(encoding='utf-8'), 'html.parser')
        for _, _, _, original in targets(soup):
            text, _ = unit(original)
            if text is not None:
                unique[hashlib.sha256(text.encode()).hexdigest()] = text
    return unique


def call_model(batch, language, glossary, endpoint, model, provider):
    instruction = (f'Translate these Avida-ED educational project pages into {language}. '
        'Return a JSON object with a translations array of strings in precisely the same order and length. '
        'Preserve every placeholder such as ⟦P0⟧, ⟦T1⟧, and ⟦N2⟧ exactly once. '
        'Keep T placeholders in their original order, and N placeholders in their original order. '
        'P placeholders may move to preserve natural grammar. They protect links, markup, names and numbers. '
        'Preserve identifiers such as demo-verify, release-views, and commit hashes exactly. '
        'Do not add commentary. Preserve scientific uncertainty '
        'and do not add claims. Run means a simulation, not duration; seed is a pseudorandom '
        'starting value; update and generation differ; exact ancestor sequence is not ancestry. '
        'Apply relevant terminology: ' + json.dumps(glossary, ensure_ascii=False))
    messages = [
        {'role': 'system', 'content': instruction},
        {'role': 'user', 'content': json.dumps(batch, ensure_ascii=False)},
    ]
    schema = {
        'type': 'object', 'properties': {'translations': {'type': 'array',
        'items': {'type': 'string'}, 'minItems': len(batch), 'maxItems': len(batch)}},
        'required': ['translations']}
    if provider == 'ollama':
        payload = {'model': model, 'messages': messages, 'think': False, 'stream': False,
                   'format': schema, 'options': {'temperature': 0, 'num_ctx': 12000}}
        route = '/api/chat'
    else:
        payload = {'model': model, 'messages': messages, 'stream': False, 'temperature': 0,
                   'max_tokens': 6000, 'chat_template_kwargs': {'enable_thinking': False},
                   'response_format': {'type': 'json_schema', 'json_schema': {'name': 'translations', 'schema': schema}}}
        route = '/v1/chat/completions'
    body = json.dumps(payload).encode()
    req = urllib.request.Request(endpoint.rstrip('/') + route, body,
                                 {'Content-Type': 'application/json'})
    with urllib.request.urlopen(req, timeout=600) as response:
        result = json.load(response)
    content = result['message']['content'] if provider == 'ollama' else result['choices'][0]['message']['content']
    decoded = json.loads(content)
    if 'translations' not in decoded:
        raise ValueError(f'model did not return translations: {str(decoded)[:500]}')
    values = decoded['translations']
    if not isinstance(values, list) or len(values) != len(batch):
        raise ValueError('translation count mismatch')
    if any(not isinstance(value, str) or not value.strip() for value in values):
        raise ValueError('model returned an empty or non-text translation')
    return values


def translate_fragmented(source, language, glossary, endpoint, model, provider):
    """Translate text between protected markers when a model drops a marker.

    This preserves the source markup and protected values exactly. Short segments
    can be less idiomatic, so the resulting catalog remains a review draft.
    """
    parts = re.split(f'({TOKENS.pattern})', source)
    translatable = [part.strip() for part in parts
                    if part and not TOKENS.fullmatch(part) and re.search(r'[A-Za-z]', part)]
    try:
        values = call_model(translatable, language, glossary, endpoint, model, provider)
    except (ValueError, KeyError, json.JSONDecodeError):
        values = [call_model([part], language, glossary, endpoint, model, provider)[0]
                  for part in translatable]
    translations = iter(values)
    result = []
    for part in parts:
        if not part or TOKENS.fullmatch(part) or not re.search(r'[A-Za-z]', part):
            result.append(part)
            continue
        leading = part[:len(part) - len(part.lstrip())]
        trailing = part[len(part.rstrip()):]
        translated = next(translations)
        if TOKENS.search(translated):
            raise ValueError('unexpected placeholder in translated fragment')
        result.append(leading + translated.strip() + trailing)
    value = ''.join(result)
    if not valid_tokens(source, value):
        raise ValueError(f'fragment translation failed placeholder validation: {source[:120]!r}')
    return value


def translate(code, source_units, args):
    path = OUTPUT / code / 'catalog.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    compendium_bytes = args.compendium.read_bytes()
    compendium = json.loads(compendium_bytes)
    if compendium.get('schema') != 'talkorigins.translation_compendium.v1':
        raise ValueError('unexpected TalkOrigins compendium schema')
    compendium_hash = hashlib.sha256(compendium_bytes).hexdigest()
    data = json.loads(path.read_text(encoding='utf-8')) if path.exists() else {'messages': {}}
    if data['messages'] and (data['metadata']['compendium_sha256'] != compendium_hash
                             or data['metadata']['model'] != args.model):
        raise ValueError(f'{code}: catalog provenance differs; use a fresh catalog')
    translations = {key: value for key, value in data['messages'].items() if key in source_units}
    glossary = {term: values[code] for term, values in compendium['terms'].items()
                if code in values and any(term.lower() in text.lower() for text in source_units.values())}
    glossary.update({
        'es': {'simulation run': 'ejecución de simulación', 'random seed': 'semilla aleatoria', 'sequence richness': 'riqueza de secuencias', 'Freezer': 'Almacén'},
        'fr': {'simulation run': 'exécution de simulation', 'random seed': 'graine aléatoire', 'sequence richness': 'richesse des séquences', 'Freezer': 'Congélateur'},
        'de': {'simulation run': 'Simulationslauf', 'random seed': 'Zufallsstartwert', 'sequence richness': 'Sequenzvielfalt', 'Freezer': 'Gefrierfach'},
        'pt': {'simulation run': 'execução de simulação', 'random seed': 'semente aleatória', 'sequence richness': 'riqueza de sequências', 'Freezer': 'Congelador'},
    }[code])
    pending = [(key, text) for key, text in source_units.items()
               if key not in translations or translations[key]['source'] != text
               or not valid_tokens(text, translations[key]['value'])]
    for offset in range(0, len(pending), args.batch_size):
        batch = pending[offset:offset + args.batch_size]
        try:
            values = call_model([x[1] for x in batch], LANGUAGES[code], glossary, args.endpoint, args.model, args.provider)
        except (ValueError, KeyError, json.JSONDecodeError):
            values = [''] * len(batch)
        for (key, source), value in zip(batch, values):
            method = 'local model draft'
            if not value or not valid_tokens(source, value):
                try:
                    value = call_model([source], LANGUAGES[code], glossary, args.endpoint, args.model, args.provider)[0]
                except (ValueError, KeyError, json.JSONDecodeError):
                    value = ''
                if not value or not valid_tokens(source, value):
                    value = translate_fragmented(source, LANGUAGES[code], glossary, args.endpoint, args.model, args.provider)
                    method = 'local model draft, translated in protected fragments'
            translations[key] = {'source': source, 'value': value, 'method': method}
        output = {'metadata': {'language': LANGUAGES[code], 'status': 'machine_translated_pending_human_review',
                  'reviewed': False, 'model': args.model, 'provider': args.provider,
                  'updated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  'terminology_source': 'TalkOrigins translation compendium',
                  'compendium_sha256': compendium_hash, 'avida_glossary': glossary}, 'messages': translations}
        tmp = path.with_suffix('.json.tmp')
        tmp.write_text(json.dumps(output, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        tmp.replace(path)
        print(f'{code}: {min(offset + len(batch), len(pending))}/{len(pending)} new units', flush=True)


def render(code, source_units):
    path = OUTPUT / code / 'catalog.json'
    translations = json.loads(path.read_text(encoding='utf-8'))['messages']
    for filename, source_path in SOURCES.items():
        soup = BeautifulSoup(source_path.read_text(encoding='utf-8'), 'html.parser')
        soup.html['lang'] = 'pt-BR' if code == 'pt' else code
        for kind, element, attr, source in list(targets(soup)):
            text, substitutions = unit(source)
            if text is None:
                continue
            key = hashlib.sha256(text.encode()).hexdigest()
            entry = translations.get(key)
            if not entry or entry['source'] != text or not valid_tokens(text, entry['value']):
                raise ValueError(f'{code}/{filename}: missing or invalid translation for {text[:100]!r}')
            value = entry['value']
            if kind == 'html':
                pieces = TOKENS.split(value)
                tokens = TOKENS.findall(value)
                restored = html.escape(pieces[0])
                mapping = dict(substitutions)
                for token, piece in zip(tokens, pieces[1:]):
                    restored += mapping[token] + html.escape(piece)
                fragment = BeautifulSoup('<body>' + restored + '</body>', 'html.parser').body
                element.clear()
                for child in list(fragment.contents):
                    element.append(child.extract())
            else:
                for token, original in substitutions:
                    value = value.replace(token, original)
                if kind == 'text':
                    element.string = value
                else:
                    element[attr] = value
        for tag in soup.find_all(['a', 'link', 'img', 'script']):
            attr = 'href' if tag.name in ('a', 'link') else 'src'
            target = tag.get(attr, '')
            if not target or target.startswith(('#', '/', 'https:', 'http:', 'mailto:', 'data:')):
                continue
            if target.startswith('app/'):
                tag[attr] = '../' + target + '?lang=' + code
            elif target.split('#', 1)[0] not in SOURCES:
                tag[attr] = '../' + target
        (OUTPUT / code / filename).write_text(str(soup), encoding='utf-8')
    print(f'{code}: rendered {len(SOURCES)} pages')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['inventory', 'translate', 'render'])
    parser.add_argument('--languages', nargs='+', choices=LANGUAGES, default=list(LANGUAGES))
    parser.add_argument('--compendium', type=Path)
    parser.add_argument('--endpoint', default='http://127.0.0.1:11434')
    parser.add_argument('--model', default='qwen3:30b')
    parser.add_argument('--provider', choices=('ollama', 'openai'), default='ollama')
    parser.add_argument('--batch-size', type=int, default=8)
    args = parser.parse_args()
    sources = load_sources()
    if args.mode == 'inventory':
        print(f'{len(SOURCES)} pages; {len(sources)} unique translation units; {sum(map(len, sources.values()))} characters')
    elif args.mode == 'translate':
        if not args.compendium:
            parser.error('translate requires --compendium')
        for code in args.languages:
            translate(code, sources, args)
    else:
        for code in args.languages:
            render(code, sources)
        if set(args.languages) == set(LANGUAGES):
            files = {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in SOURCES.items()}
            pages = {code: {name: hashlib.sha256((OUTPUT / code / name).read_bytes()).hexdigest()
                            for name in SOURCES} for code in LANGUAGES}
            catalogs = {code: hashlib.sha256((OUTPUT / code / 'catalog.json').read_bytes()).hexdigest()
                        for code in LANGUAGES}
            (OUTPUT / 'manifest.json').write_text(json.dumps({
                'schema': 1, 'source_sha256': files, 'page_sha256': pages, 'catalog_sha256': catalogs
            }, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
