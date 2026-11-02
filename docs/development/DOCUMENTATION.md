# Maintaining documentation

Keep README's project scope and current build instructions concise. User guides in docs/user describe current behavior, without release-by-release overrides. Developer references in docs/development describe current file formats, build/test procedures and distribution. Use CHANGELOG for dated feature history and Git history for superseded plans and test handoffs.

When changing a persisted field, update SESSION_FORMAT and its compatibility/migration notes. When changing CTest entries, update the expected count in README, BUILDING and TESTING. Record native acceptance by exact application/installer version; do not carry an earlier pass forward as proof of changed behavior. Update relative links when moving documents and run `python3 tools/check_doc_links.py` before packaging.
