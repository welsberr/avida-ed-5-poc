#!/usr/bin/env python3
"""Check generated site language routes, preserved anchors and local resources."""
import importlib.util
import shutil
import tempfile
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('site_assembler', ROOT / 'scripts/assemble_localized_site.py')
assembler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assembler)


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.references = []
        self.language = ''
        self.current_languages = 0
        self.language_navigation = 0
        self.anchor_text = None
        self.empty_anchors = []

    def handle_starttag(self, tag, attributes):
        attr = dict(attributes)
        if tag == 'html':
            self.language = attr.get('lang', '')
        if 'id' in attr:
            if attr['id'] in self.ids:
                raise AssertionError(f'duplicate id {attr["id"]}')
            self.ids.add(attr['id'])
        if tag == 'nav' and 'site-language-bar' in attr.get('class', '').split():
            self.language_navigation += 1
        if tag == 'a' and attr.get('aria-current') == 'page':
            self.current_languages += 1
        if tag == 'a':
            self.anchor_text = ''
        if tag == 'img' and self.anchor_text is not None:
            self.anchor_text += attr.get('alt', '')
        for key in ('href', 'src'):
            if key in attr:
                self.references.append(attr[key])

    def handle_data(self, data):
        if self.anchor_text is not None:
            self.anchor_text += data

    def handle_endtag(self, tag):
        if tag == 'a' and self.anchor_text is not None:
            if not self.anchor_text.strip():
                self.empty_anchors.append('empty link')
            self.anchor_text = None


def parse(path):
    page = Page()
    page.feed(path.read_text(encoding='utf-8'))
    return page


def main():
    assembler.validate()
    with tempfile.TemporaryDirectory(prefix='avida-site-test-') as tmp:
        root = Path(tmp)
        # The production workflow copies the app and evidence separately.
        (root / 'app').mkdir()
        (root / 'app/Avida.html').write_text('<!doctype html><title>test</title>', encoding='utf-8')
        shutil.copytree(ROOT / 'docs/evidence', root / 'evidence')
        for code in assembler.LANGUAGES:
            destination = root if code == 'en' else root / code
            destination.mkdir(exist_ok=True)
            for name, source in assembler.SOURCES.items():
                input_file = source if code == 'en' else assembler.TRANSLATIONS / code / name
                (destination / name).write_text(assembler.add_navigation(
                    input_file.read_text(encoding='utf-8'), name, code), encoding='utf-8')
        shutil.copy2(ROOT / 'docs/site-report.css', root / 'site-report.css')
        for name in assembler.SOURCES:
            english = parse(root / name)
            external_links = {ref for ref in english.references if urlsplit(ref).scheme in ('http', 'https')}
            for code in assembler.LANGUAGES:
                path = root / name if code == 'en' else root / code / name
                page = parse(path)
                expected_lang = 'pt-BR' if code == 'pt' else code
                assert page.language == expected_lang, (path, page.language)
                assert page.language_navigation == 1 and page.current_languages == 1, path
                assert not page.empty_anchors, (path, page.empty_anchors)
                assert page.ids == english.ids, (path, 'anchors changed')
                assert external_links == {ref for ref in page.references if urlsplit(ref).scheme in ('http', 'https')}, path
                if name == 'index.html' and code != 'en':
                    assert f'../app/Avida.html?lang={code}' in page.references, path
                for reference in page.references:
                    url = urlsplit(reference)
                    if url.scheme or url.netloc or url.path.startswith('/'):
                        continue
                    target = (path.parent / unquote(url.path)).resolve() if url.path else path
                    assert target.is_relative_to(root.resolve()) and target.is_file(), (path, reference)
                    if url.fragment and target.suffix == '.html' and target.name != 'Avida.html':
                        assert unquote(url.fragment) in parse(target).ids, (path, reference)
    print('validated page languages, navigation, links, resources, and anchors')


if __name__ == '__main__':
    main()
