# pinyin-data snapshot

This directory vendors source data from the upstream `mozillazg/pinyin-data` project.

- Upstream: https://github.com/mozillazg/pinyin-data
- Snapshot file used: `pinyin.txt`
- License: MIT (see `LICENSE`)

Usage in this project:
- `scripts/generate_pinyin_map.py` reads `pinyin.txt`.
- By default generation uses all characters available in `pinyin.txt`.
- `--required-chars /path/to/required_chars.txt` can be passed as an optional lock to constrain output.
- Generated output is `include/phonefinder/pinyin_map_generated.hpp`.
