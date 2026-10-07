#!/usr/bin/env python3
"""Check public release identity and source distribution safety."""
from pathlib import Path
import json,re,subprocess
root=Path(__file__).resolve().parents[1]
version=(root/'VERSION').read_text().strip()
assert re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-zA-Z0-9.]+)?',version),'Invalid VERSION'
assert json.loads((root/'ats-dualsense/desktop/package.json').read_text())['version']==version
assert f'HaulSense {version}' in (root/'ats-dualsense/ui/index.html').read_text()
assert "script-src 'self' 'unsafe-inline'" not in (root/'ats-dualsense/daemon/src/dashboard.cpp').read_text()
for html in ['index.html','hud.html']:
 assert '<script>' not in (root/'ats-dualsense/ui'/html).read_text(),'Executable inline script breaks strict CSP'
repository=subprocess.run(['git','rev-parse','--show-toplevel'],cwd=root,text=True,capture_output=True)
tracked=subprocess.check_output(['git','ls-files'],cwd=root,text=True).splitlines() if repository.returncode==0 and Path(repository.stdout.strip()).resolve()==root.resolve() else []
if not tracked:print('Source distribution: Git index hygiene is checked in repository CI.')
assert not any(p.startswith(('build/','node_modules/','ats-dualsense/desktop/node_modules/')) or p.endswith(('.dll','.deb')) for p in tracked),'Generated/private artifacts tracked'
print(f'Release identity and source hygiene passed: {version}')
