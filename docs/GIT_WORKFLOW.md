# Initialize the Kleiotide GitHub repository (Windows)

This archive is the **source tree**, not a Git repository. It has no `.git` folder and contains no connection to your private GitHub account. The `Kleiotide_0.28.0/` folder is the repository root: Git should track the files **inside** that folder, not a single ZIP file.

If your repository already contains 0.26.11, update that checkout with these source
files, inspect `git diff` and make a normal commit. Do not initialize a second
repository or force-push over existing history. No remote operation was performed
while preparing this archive.

## Initial import into an empty private GitHub repository

1. Extract `Kleiotide_0.28.0_GitHub_Ready.zip` and open **PowerShell** in the extracted `Kleiotide_0.28.0` directory (the one containing `CMakeLists.txt` and `.gitignore`).
2. Run the following commands, replacing `YOUR_ACCOUNT/YOUR_REPOSITORY` with the address shown on the empty repository's GitHub page:

```powershell
git --version
git init
git add .
git status --short
git commit -m "Import Kleiotide 0.28.0 source"
git branch -M main
git remote add origin "https://github.com/YOUR_ACCOUNT/YOUR_REPOSITORY.git"
git push -u origin main
```

If Git requests an author identity for the first commit, set it locally and repeat the commit:

```powershell
git config user.name "YOUR NAME"
git config user.email "YOUR-EMAIL@example.com"
```

Use an email address you are comfortable associating with commits (GitHub provides private noreply addresses). GitHub HTTPS push uses an approved credential method such as Git Credential Manager, not your GitHub account password. The first push may open a sign-in dialog.

3. Verify on GitHub that the root shows `src/`, `tests/`, `tools/`, `CMakeLists.txt`, `LICENSE`, and `README.md`. **Do not push a ZIP archive in place of the source files.**

The provided `.gitignore` excludes generated installers, build directories, local Qt Creator settings and ZIPs. If you run `git add .` after building, check `git status` before committing.

## Day-to-day development

Continue editing this checked-out folder in **Qt Creator**. For a new change:

```powershell
git status
git diff
git add .
git commit -m "Describe the change"
git push
```

For a new release, complete the build/test checklist and then add an annotated tag:

```powershell
git tag -a v0.28.0 -m "Kleiotide 0.28.0 testing update"
git push origin v0.28.0
```

Tagging 0.28.0 is optional; it is best done after confirming this snapshot matches your accepted local sources. GitHub **Releases** and distributable installers should be created only after their separate acceptance and dependency checks. A repository push does not build or upload an installer.

## Building a matched source ZIP for the Windows test-installer script

`tools/windows/make-installer.ps1` expects an exact versioned source ZIP and checks its files against the working tree before embedding it. If you use the GitHub-ready tree rather than the older installer-preparation source ZIP, **regenerate the matched source ZIP** after any tracked non-hidden source/documentation change:

```powershell
python .\tools\package_source.py "..\Kleiotide_0.28.0_Source_For_Installer.zip"
```

Use the resulting ZIP as `KLEIOTIDE_SOURCE_ARCHIVE` in Qt Creator. It has the required internal `Kleiotide_0.28.0/` prefix. The GitHub-ready ZIP is intended for import into Git, not necessarily as the exact installer-embedded source archive. Third-party runtime licenses and sources remain governed by [WINDOWS_INSTALLER.md](../WINDOWS_INSTALLER.md).

## Notes

- Initial GitHub history starts with this snapshot, not with the older standalone development ZIPs.
- Repository visibility can remain private during colleague testing; making it public later will expose all committed history.
- Before publication, verify the licensing terms, public-facing information and any third-party data/attribution that will be visible.
