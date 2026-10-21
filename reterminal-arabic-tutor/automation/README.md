# Daily automation setup

This makes new content appear on your reTerminal Sticky every day with zero
manual steps, using free GitHub infrastructure.

## 1. Put this whole project in a GitHub repo

```
cd reterminal-arabic-tutor
git init
git add .
git commit -m "Initial commit"
gh repo create reterminal-arabic-tutor --public --source=. --push
# (or create the repo on github.com and `git push` manually)
```

Move the workflow file to the repo root so GitHub picks it up:
```
mkdir -p .github/workflows
mv automation/.github/workflows/daily-content.yml .github/workflows/
```

## 2. Add your API key as a secret

Repo → Settings → Secrets and variables → Actions → New repository secret:
- Name: `ANTHROPIC_API_KEY`
- Value: your own key from console.anthropic.com

This is **your** key and **your** billing — the Action calls the API once a
day with a short prompt, so cost is negligible, but it's not free/unlimited.

## 3. Enable GitHub Pages

Repo → Settings → Pages → Source: "Deploy from a branch" → Branch: `main`,
folder: `/docs`. After the first successful Action run, your content will be
live at:

```
https://<your-username>.github.io/reterminal-arabic-tutor/content/latest/manifest.json
```

## 4. Point the device at it

Put that exact URL into `firmware/src/config.h`'s `CONTENT_MANIFEST_URL`,
rebuild, reflash.

## 5. Test it without waiting a day

Actions tab → "Generate daily Arabic tutor content" → "Run workflow" to
trigger it on demand instead of waiting for the 05:00 UTC schedule.

## Changing the schedule or topics

- Schedule: edit the `cron` line in the workflow file.
- Topics: `content-pipeline/generate_day.py`'s `default_topics()` list — add,
  remove, or reorder as you like. Pass `--topic "something specific"` to the
  script (or add a `workflow_dispatch` input) if you want to force a topic
  for a given run instead of the rotation.
