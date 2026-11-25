#!/usr/bin/env python3
"""Validate translation fingerprints and assemble static pages for GitHub Pages."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
TRANSLATIONS = ROOT / 'docs/site-locales'
SOURCES = {
    'index.html': ROOT / 'index.html',
    **{name: ROOT / 'docs' / name for name in (
        'port-status.html', 'codex-collaboration.html', 'porting-learning-roadmap.html',
        'roadmap.html', 'visual-audit-2026-09-30.html')},
}
LANGUAGES = {'en': 'English', 'es': 'Español', 'fr': 'Français', 'de': 'Deutsch', 'pt': 'Português'}
LABELS = {'en': 'Page language', 'es': 'Idioma de la página', 'fr': 'Langue de la page',
          'de': 'Seitensprache', 'pt': 'Idioma da página'}
NOTICES = {
    'es': 'Traducción asistida por máquina; pendiente de revisión humana.',
    'fr': 'Traduction assistée par machine, en attente de révision humaine.',
    'de': 'Maschinell unterstützte Übersetzung; eine menschliche Prüfung steht noch aus.',
    'pt': 'Tradução assistida por máquina, pendente de revisão humana.',
}
CSS = '''<style>
.site-language-bar { background:#fff; color:#172b3b; border-bottom:1px solid #d8e0e6; padding:.55rem 1rem; font: .9rem/1.5 system-ui,sans-serif; }
.site-language-inner { max-width:70rem; margin:auto; display:flex; flex-wrap:wrap; align-items:center; gap:.3rem .8rem; }
.site-language-bar ul { display:flex; flex-wrap:wrap; gap:.25rem .8rem; list-style:none; margin:0; padding:0; }
.site-language-bar a { color:#075b86; text-decoration:underline; } .site-language-bar [aria-current] { font-weight:700; color:#172b3b; }
.site-translation-notice { max-width:70rem; margin:.2rem auto 0; color:#435565; font-size:.84rem; }
</style>'''


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate():
    manifest = json.loads((TRANSLATIONS / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('schema') != 1 or set(manifest['source_sha256']) != set(SOURCES):
        raise ValueError('site localization manifest has an unsupported schema or page inventory')
    if set(manifest['page_sha256']) != set(LANGUAGES) - {'en'}:
        raise ValueError('site localization manifest is missing a language')
    for name, path in SOURCES.items():
        if checksum(path) != manifest['source_sha256'][name]:
            raise ValueError(f'{name} changed; refresh its translations before deploying')
    for code in set(LANGUAGES) - {'en'}:
        if set(manifest['page_sha256'][code]) != set(SOURCES):
            raise ValueError(f'{code}: incomplete page inventory')
        if checksum(TRANSLATIONS / code / 'catalog.json') != manifest['catalog_sha256'][code]:
            raise ValueError(f'{code}: translation catalog changed; render the localized pages again')
        for name in SOURCES:
            if checksum(TRANSLATIONS / code / name) != manifest['page_sha256'][code][name]:
                raise ValueError(f'{code}/{name}: localized page changed; render again')


def add_navigation(page, filename, language):
    links = []
    for code, label in LANGUAGES.items():
        if language == 'en':
            url = filename if code == 'en' else f'{code}/{filename}'
        else:
            url = f'../{filename}' if code == 'en' else f'../{code}/{filename}'
        current = ' aria-current="page"' if code == language else ''
        hreflang = 'pt-BR' if code == 'pt' else code
        links.append(f'<li><a href="{url}" lang="{hreflang}" hreflang="{hreflang}"{current}>{label}</a></li>')
    notice = f'<p class="site-translation-notice">{NOTICES[language]}</p>' if language != 'en' else ''
    nav = (f'<nav class="site-language-bar" aria-label="{LABELS[language]}">'
           f'<div class="site-language-inner"><strong>{LABELS[language]}:</strong><ul>{"".join(links)}</ul></div>'
           f'{notice}</nav>')
    if page.count('</head>') != 1 or page.count('<body>') != 1:
        raise ValueError(f'{language}/{filename}: unexpected document structure')
    return page.replace('</head>', CSS + '</head>', 1).replace('<body>', '<body>' + nav, 1)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, help='assemble pages into this site directory')
    args = parser.parse_args()
    validate()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=True)
        for code in LANGUAGES:
            destination = args.output if code == 'en' else args.output / code
            destination.mkdir(parents=True, exist_ok=True)
            for name, source in SOURCES.items():
                input_file = source if code == 'en' else TRANSLATIONS / code / name
                rendered = add_navigation(input_file.read_text(encoding='utf-8'), name, code)
                (destination / name).write_text(rendered, encoding='utf-8')
        shutil.copy2(ROOT / 'docs/site-report.css', args.output / 'site-report.css')
    print(f'validated {len(SOURCES)} pages in {len(LANGUAGES)} languages')


if __name__ == '__main__':
    main()
