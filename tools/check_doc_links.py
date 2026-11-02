"""Check local Markdown links and heading anchors without network requests."""
from pathlib import Path
import re,sys,urllib.parse
root=Path(__file__).resolve().parents[1]
def anchors(text):
 found=set();seen={}
 for title in re.findall(r'^#{1,6}\s+(.+?)\s*#*$',text,re.M):
  value=re.sub(r'[^\w\- ]','',title.lower()).replace(' ','-');n=seen.get(value,0);seen[value]=n+1
  found.add(value+('-'+str(n) if n else ''))
 return found
errors=[];checked=0
for path in sorted(root.rglob('*.md')):
 if any(p in {'build','.git','third_party'} or p.startswith('build-') for p in path.relative_to(root).parts):continue
 text=path.read_text();text=re.sub(r'```.*?```','',text,flags=re.S)
 for target in re.findall(r'(?<!!)\[[^\]]*\]\(([^)]+)\)',text):
  target=target.split(' "',1)[0].strip('<>');url=urllib.parse.urlsplit(target)
  if url.scheme or target.startswith('//'):continue
  checked+=1;dest=(path.parent/urllib.parse.unquote(url.path)).resolve() if url.path else path
  if not dest.exists():errors.append(f'{path.relative_to(root)}: missing {target}')
  elif url.fragment and dest.suffix.lower()=='.md' and urllib.parse.unquote(url.fragment) not in anchors(dest.read_text()):errors.append(f'{path.relative_to(root)}: missing anchor {target}')
if errors:print('\n'.join(errors));sys.exit(1)
print(f'PASS {checked} local Markdown links/anchors')
