#!/usr/bin/env python3
"""Generate unreviewed UI translation candidates using a local Ollama endpoint.

The Spanish catalog supplies the source inventory; no translation is treated as
reviewed. Each batch is checked for length and exact placeholder preservation.
Existing entries in the target draft allow a stopped run to resume.
"""
import argparse
import datetime
import hashlib
import json
import re
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'docs/locales/es/draft-catalog.json'
TOKEN = re.compile(r'\{[A-Za-z_][A-Za-z0-9_]*\}')
LANGUAGES = {'fr': 'French', 'de': 'German', 'pt': 'Brazilian Portuguese'}
AVIDA_TERMS = {
    'fr': {'simulation run': 'exécution de simulation', 'random seed': 'graine aléatoire',
           'update (simulation step)': 'mise à jour', 'freezer (storage)': 'congélateur',
           'sequence richness': 'richesse des séquences', 'ancestor sequence': 'séquence ancestrale',
           'fitness (biology)': 'valeur sélective'},
    'de': {'simulation run': 'Simulationslauf', 'random seed': 'Zufallsstartwert',
           'update (simulation step)': 'Aktualisierung', 'freezer (storage)': 'Gefrierfach',
           'sequence richness': 'Sequenzvielfalt', 'ancestor sequence': 'Ahnensequenz',
           'fitness (biology)': 'Fitness'},
    'pt': {'simulation run': 'execução de simulação', 'random seed': 'semente aleatória',
           'update (simulation step)': 'atualização', 'freezer (storage)': 'congelador',
           'sequence richness': 'riqueza de sequências', 'ancestor sequence': 'sequência ancestral',
           'fitness (biology)': 'aptidão'},
}


def translate(batch, language, endpoint, model, glossary):
    sources = [item['source'] for item in batch]
    system = (f'Translate each English string for an educational digital evolution application into {language}. '
              'Return JSON with a translations array of strings in exactly the same order and length. '
              'Keep placeholders such as {update} exactly unchanged. Preserve Avida-ED, Avida 5, '
              'logic-task codes and gene instruction names. Distinguish simulation update from generation, '
              'and genome sequence from ancestry. Run means a simulation experiment, not its duration. '
              'Seed means a pseudorandom starting value, not a biological seed. '
              'Use concise interface wording. Output JSON only. '
              'Use these TalkOrigins scientific terminology suggestions when applicable: '
              + json.dumps(glossary, ensure_ascii=False))
    data = json.dumps({'model': model, 'messages': [
        {'role': 'system', 'content': system},
        {'role': 'user', 'content': json.dumps(sources, ensure_ascii=False)},
    ], 'think': False, 'stream': False, 'format': 'json',
       'options': {'temperature': 0, 'num_ctx': 8192}}).encode()
    request = urllib.request.Request(endpoint.rstrip('/') + '/api/chat', data,
                                     {'Content-Type': 'application/json'})
    with urllib.request.urlopen(request, timeout=300) as response:
        result = json.load(response)
    values = json.loads(result['message']['content'])['translations']
    if not isinstance(values, list) or len(values) != len(batch):
        raise ValueError('translation count mismatch')
    for source, value in zip(sources, values):
        if not isinstance(value, str) or not value.strip() or sorted(TOKEN.findall(source)) != sorted(TOKEN.findall(value)):
            raise ValueError(f'invalid translation or placeholders for {source!r}')
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('languages', nargs='+', choices=LANGUAGES)
    parser.add_argument('--endpoint', default='http://127.0.0.1:11434')
    parser.add_argument('--model', default='qwen3:30b')
    parser.add_argument('--batch-size', type=int, default=20)
    parser.add_argument('--compendium', type=Path, required=True, help='TalkOrigins translation compendium')
    parser.add_argument('--restart', action='store_true', help='regenerate an existing draft')
    args = parser.parse_args()
    messages = json.loads(SOURCE.read_text(encoding='utf-8'))['messages']
    compendium_bytes = args.compendium.read_bytes()
    compendium = json.loads(compendium_bytes)
    if compendium.get('schema') != 'talkorigins.translation_compendium.v1':
        raise ValueError('unexpected TalkOrigins compendium schema')
    compendium_hash = hashlib.sha256(compendium_bytes).hexdigest()
    for code in args.languages:
        path = ROOT / 'docs/locales' / code / 'draft-catalog.json'
        path.parent.mkdir(parents=True, exist_ok=True)
        saved = json.loads(path.read_text(encoding='utf-8')) if path.exists() and not args.restart else {'messages': []}
        if saved['messages'] and (saved['metadata'].get('compendium_sha256') != compendium_hash
                                  or saved['metadata'].get('model') != args.model
                                  or saved['metadata'].get('avida_glossary') != AVIDA_TERMS[code]):
            raise ValueError(f'{code}: draft provenance changed; use --restart to regenerate')
        glossary = {term: translations[code] for term, translations in compendium['terms'].items()
                    if code in translations and any(term.lower() in item['source'].lower() for item in messages)}
        glossary.update(AVIDA_TERMS[code])
        done = {item['id']: item for item in saved['messages']}
        pending = [item for item in messages if item['id'] not in done or done[item['id']]['source'] != item['source']]
        for start in range(0, len(pending), args.batch_size):
            batch = pending[start:start + args.batch_size]
            try:
                values = translate(batch, LANGUAGES[code], args.endpoint, args.model, glossary)
            except (ValueError, KeyError, json.JSONDecodeError):
                # A smaller request isolates malformed model output without losing progress.
                values = [translate([item], LANGUAGES[code], args.endpoint, args.model, glossary)[0] for item in batch]
            for item, value in zip(batch, values):
                done[item['id']] = {'id': item['id'], 'source': item['source'], 'value': value}
            result = {'metadata': {'language': LANGUAGES[code], 'locale': code,
                'status': 'machine_translated_pending_human_review', 'reviewed': False,
                'model': args.model, 'compendium_sha256': compendium_hash,
                'avida_glossary': AVIDA_TERMS[code],
                'terminology_source': 'TalkOrigins translation compendium',
                'updated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'note': 'Local model draft; scientific terminology and visible layout require human review.'},
                'messages': [done[item['id']] for item in messages if item['id'] in done]}
            tmp = path.with_suffix('.json.tmp')
            tmp.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
            tmp.replace(path)
            print(f'{code}: {len(result["messages"])}/{len(messages)}', flush=True)


if __name__ == '__main__':
    main()
