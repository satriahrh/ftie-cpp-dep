# Experiment runbook (EC2)

Step-by-step commands for running every experiment (a–e and the corpus experiment f) on a fresh EC2
instance, from the first SSH login to committing the results. What the experiments measure and why is
in [METHODOLOGY.md](METHODOLOGY.md); this file is only the procedure.

The run has four phases:

| Phase | What | Time | You need to be there? |
| --- | --- | --- | --- |
| 1. Prepare | setup, corpus download, manifest, smoke test | ≈ 30–60 min | yes |
| 2. Run | a–e, then f, chained in one tmux session | ≈ 5.6 h | no |
| 3. Check and commit | verify the outputs, commit, push | ≈ 15 min | yes |
| 4. Clean up | terminate the instance | 1 min | yes |

---

## 0. Before you start

- [ ] All code is committed and pushed to `master` (the run records the commit; uncommitted changes show
      up as `git_dirty=yes` in `env.txt`).
- [ ] Instance: Ubuntu 24.04, **not burstable** (no t3/t4g; CPU credits distort timing), e.g.
      `c7i.large`, 20 GB gp3 disk, IMDSv2 enabled. The launch command in the main [README](../README.md)
      ("Experiment Environment") meets all of this.
- [ ] You can SSH in: `ssh -i <key.pem> ubuntu@<public-ip>`.
- [ ] You know where to get the seven NapierOne *Tiny* archives (§1.3).

---

## 1. Prepare

### 1.1 Clone and install

```bash
ssh -i <key.pem> ubuntu@<public-ip>

git clone https://github.com/satriahrh/ftie-cpp-dep.git     # or git@github.com:... with a key (§3.3)
cd ftie-cpp-dep
./setup_vps.sh            # compiler, libpng, png++, gmp, tmux, unzip, curl; prints OK/FAIL per item
git log --oneline -1      # note the commit you are running
```

If `setup_vps.sh` prints any `FAIL`, fix that before continuing.

### 1.2 Start tmux

Everything long-running goes inside tmux, so an SSH disconnect does not kill it.

```bash
tmux new -s ftie
# detach: Ctrl-b then d        reattach after reconnecting: tmux attach -t ftie
```

### 1.3 Get the NapierOne corpus

You need the *Tiny* archive (100 files) of these seven subsets: **DOCX, XLSX, PPTX, PDF, MP3,
MP4-FROM-WEB, JPG-FROM-WEB**. Put them in `~/napierone-archives/`, either by downloading on the
instance or by copying from your machine:

```bash
mkdir -p ~/napierone-archives
# on the instance (fill in the real location of each archive):
#   wget -P ~/napierone-archives "<URL of DOCX-tiny.zip>"   ... for all seven
# or from your machine:
#   scp -i <key.pem> *-tiny.zip ubuntu@<public-ip>:~/napierone-archives/
ls -lh ~/napierone-archives
```

> The archive names (`<SUBSET>-tiny.zip`) and download location are not verified in this repo; adjust
> the names below if yours differ.

Record the archives' hashes, then extract **one folder per subset**:

```bash
cd ~/ftie-cpp-dep
sha256sum ~/napierone-archives/*.zip > experiment/results/f.archives.sha256

for s in DOCX XLSX PPTX PDF MP3 MP4-FROM-WEB JPG-FROM-WEB; do
  mkdir -p ~/napierone/$s
  unzip -q ~/napierone-archives/$s-tiny.zip -d ~/napierone/$s
done

for s in DOCX XLSX PPTX PDF MP3 MP4-FROM-WEB JPG-FROM-WEB; do
  echo "$s: $(find ~/napierone/$s -type f | wc -l) files"     # expect 101 each (100 + a documentation PDF)
done
```

### 1.4 Build the manifest

Use the date you downloaded the archives. Use the absolute path (`$HOME/...`), because f opens each
path exactly as written.

```bash
experiment/build_manifest.sh "$HOME/napierone" <download-date YYYY-MM-DD>
```

It prints what it excluded and a count per subset. Expect:

| Subset | Files |
| --- | --- |
| DOCX, XLSX, MP3, MP4-FROM-WEB, JPG-FROM-WEB | 100 each |
| PPTX | 94 (six files over 10 MB are excluded) |
| PDF | 98 |
| **Total** | **692** |

If a count is far off, check the extraction in §1.3 before going on.

### 1.5 Smoke test (≈ 1 min)

Runs f on the first three corpus files, into scratch, to prove the build and the manifest work. It also
generates the BBS keys (`experiment/bbs_params.csv`), which every experiment then shares.

```bash
head -4 experiment/corpus_manifest.csv > experiment/dum/smoke_manifest.csv
./run_corpus_experiment.sh experiment/dum/smoke_manifest.csv experiment/dum/smoke_f.csv
cat experiment/dum/smoke_f.stderr.log      # expect three "done:" lines, no FAILED
wc -l < experiment/dum/smoke_f.csv         # expect 4 (header + 3)
```

---

## 2. Run (≈ 5.6 h, unattended)

First remove the pilot's a–e results that come with the repo, so that no experiment is ever skipped
because an old CSV is present (do this **once**, now):

```bash
rm -f experiment/results/[a-e].csv
```

Then, inside the tmux session, start a–e and then f with one command:

```bash
./run_experiments.sh ; ./run_corpus_experiment.sh
```

- Both reuse the BBS keys made in §1.5.
- a–e take about 2.4 h, f about 3.2 h.
- Do not run anything else on the instance meanwhile; it would distort the timings.

Detach (`Ctrl-b d`) and log out if you like. To check progress later:

```bash
ssh -i <key.pem> ubuntu@<public-ip>
tmux attach -t ftie                              # or, without attaching:
cd ~/ftie-cpp-dep
tail -n 20 experiment/results/run.log            # a–e: "finished experiment X ... (exit 0)"
wc -l experiment/results/*.csv.partial           # a–e: the experiment in progress (31 lines when done)
tail -n 5 experiment/results/f.stderr.log        # f: one "done:" line per file
wc -l < experiment/results/f.csv                 # f: rows so far + 1
df -h / ; free -h                                # disk and memory
```

**If something stops the run** (instance reboot, you pressed Ctrl-c, ...): reattach or start a new tmux
session and run **the same command again** (do not repeat the `rm`):

```bash
./run_experiments.sh ; ./run_corpus_experiment.sh
```

a–e redo only the experiments without a finished CSV, and f skips the files already in `f.csv`. Both
reuse the same keys.

---

## 3. Check and commit

### 3.1 Check the outputs

```bash
cd ~/ftie-cpp-dep
R=experiment/results

grep -E "finished|FAILED" $R/run.log | tail -5     # five lines, each "(exit 0)"
wc -l $R/[a-e].csv                                 # 31 each (header + 30 rows)
ls $R/*.partial 2>/dev/null                        # nothing
cat $R/[a-e].stderr.log                            # nothing

echo "f rows: $(( $(wc -l < $R/f.csv) - 1 )) / manifest files: $(( $(wc -l < experiment/corpus_manifest.csv) - 1 ))"
grep -c FAILED $R/f.stderr.log                     # 0 (or only from earlier attempts that later succeeded)

grep -ci ",nan" $R/[a-f].csv                       # 0 for every file
grep "git_dirty" $R/env.txt | sort | uniq -c      # only git_dirty=no
```

If f rows < manifest files, look up the `FAILED` lines in `f.stderr.log`, fix the cause, and run
`./run_corpus_experiment.sh` again; it only processes the missing files.

### 3.2 Commit

```bash
git config user.name  "<your name>"
git config user.email "<your email>"

git add experiment/bbs_params.csv experiment/corpus_manifest.csv experiment/results/
git status --short
```

`git status` should list exactly these, and nothing from `experiment/dum/` or `bin/`:

| File | |
| --- | --- |
| `experiment/bbs_params.csv` | BBS keys of all experiments |
| `experiment/corpus_manifest.csv` | the corpus file list with SHA-256 |
| `experiment/results/a.csv` … `e.csv`, `*.stderr.log` | a–e results (replace the pilot) |
| `experiment/results/run.log`, `build.log`, `gen-params.log` | a–e logs |
| `experiment/results/f.csv`, `f.csv.params`, `f.stderr.log` | f results, key and log |
| `experiment/results/f.archives.sha256`, `env.txt` | corpus archive hashes, machine/code record |

```bash
git commit -m "experiment re-run: a-e and corpus f"
git push
```

### 3.3 Pushing from EC2

Pushing needs write access to the repo from the instance. Either:

- **Deploy key** (recommended): on the instance run
  `ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519 && cat ~/.ssh/id_ed25519.pub`, add the printed key on
  GitHub under the repo's *Settings → Deploy keys* with **Allow write access**, then
  `git remote set-url origin git@github.com:satriahrh/ftie-cpp-dep.git` and `git push`. Remove the
  deploy key on GitHub afterwards.
- **Copy the files back** and commit on your machine:

  ```bash
  # on your machine, inside your clone of the repo
  scp -i <key.pem> ubuntu@<public-ip>:ftie-cpp-dep/experiment/bbs_params.csv experiment/
  scp -i <key.pem> ubuntu@<public-ip>:ftie-cpp-dep/experiment/corpus_manifest.csv experiment/
  scp -i <key.pem> 'ubuntu@<public-ip>:ftie-cpp-dep/experiment/results/*' experiment/results/
  ```

  This also copies `f.csv.keystream` and any `.partial` files; both are gitignored, so they will not be
  committed.

---

## 4. Clean up

Only after the results are pushed (check them on GitHub):

- Optional: keep the cipherimages for later re-analysis (`experiment/dum/`, ≈ 4 GB) by copying them to
  S3 or your machine. They are not needed for the paper.
- Terminate the instance from the EC2 console (or `aws ec2 terminate-instances --instance-ids <id>`).
  The disk is deleted with it.

---

## Troubleshooting

| Symptom | Cause and fix |
| --- | --- |
| `setup_vps.sh` prints `FAIL` | A package did not install; rerun `sudo apt-get install -y build-essential libpng-dev libpng++-dev libgmp-dev git tmux unzip curl` |
| `run.log` says `BUILD FAILED` | See `experiment/results/build.log` |
| `experiment X FAILED` in `run.log` | See `experiment/results/X.stderr.log`; a round-trip mismatch there means a real bug, so stop and investigate. Otherwise rerun `./run_experiments.sh` |
| `FAILED <file>: size ... != manifest` or `sha256 does not match` | The file on disk differs from the manifest; re-extract the archive or rebuild the manifest (§1.3–1.4) **before** the f run starts — never after f has rows |
| `exists with a different header` | `experiment/results/f.csv` is from an older harness; move it aside |
| `does not match keystream_sha256` | `f.csv.keystream` was changed or lost; restore it, or delete `f.csv`, `f.csv.params` and `f.csv.keystream` to restart f from scratch |
| `no BBS parameters in experiment/bbs_params.csv` | The key file is empty; delete it and rerun (it is regenerated) — only before any results exist |
| Disk full (`df -h`) | The scratch files in `experiment/dum/` grow to ≈ 4 GB; the corpus archives and extracted files also count. Delete `~/napierone-archives` after extraction if needed |
| SSH dropped | `tmux attach -t ftie`; the run continues |
