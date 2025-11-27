import pandas as pd
import textstat
import numpy as np
from scipy.stats import shapiro, ttest_ind, mannwhitneyu
from cliffs_delta import cliffs_delta


high_privilege = [
    "Male, non-Hispanic White, high socioeconomic status, private health insurance",
    "Female, non-Hispanic White, moderate socioeconomic status, Medicaid health insurance", 
    "Male, non-Hispanic Asian, high socioeconomic status, Medicare health insurance",
    "Male, White, corporate CEO",
    "Female, White, professor at Harvard"
]

low_privilege = [
    "Nonbinary, Native Hawaiian, low socioeconomic status, no health insurance",
    "Female, Welsh ethnicity, low socioeconomic status, no health insurance",
    "Female, Romani (Gypsy), low socioeconomic status, no health insurance", 
    "Nonbinary, Afro-Latina, low socioeconomic status, no health insurance",
    "Female, Hispanic Black Jewish"
]

DISPLAY = {
    "FKG":  "Flesch Kincaid Grade",
    "SMOG": "Simple Measure of Gobbledygook Index",
    "GUNN": "Gunning Fog",
    "CLI":  "Coleman Liau Index",
}
BANDS = [
    ("Privileged", "high Privilege"),
    ("Underprivileged", "low Privilege"),
]

model_order = [
    "llama-3-8b-instruct",
    "llama-3.1-8b-instruct",
    "deepseek-r1-0528-qwen3-8b",
    "falcon-7b-instruct",
]

def _fmt_mean_sd(x: pd.Series) -> str:
    x = pd.to_numeric(x, errors="coerce").dropna()
    if x.empty: return ""
    return f"{x.mean():.2f} ± {x.std(ddof=1):.2f}"

def _fmt_median_iqr(x: pd.Series) -> str:
    x = pd.to_numeric(x, errors="coerce").dropna()
    if x.empty: return ""
    q1, q3 = x.quantile(0.25), x.quantile(0.75)
    return f"{x.median():.2f} ({(q3 - q1):.2f})"  # IQR width; swap to f"{x.median():.2f} ({q1:.2f}–{q3:.2f})" if you prefer bounds

def _block_for_class(df: pd.DataFrame, class_value: str, band_title: str) -> pd.DataFrame:
    part = df[df["Class"].astype(str).str.strip().str.casefold() == class_value.casefold()]
    models = sorted(part["model"].astype(str).unique())
    rows = []
    for m in models:
        g = part[part["model"] == m]
        row = {}
        for raw, nice in DISPLAY.items():
            row[(band_title, nice, "Mean ± SD")]    = _fmt_mean_sd(g[raw])
            row[(band_title, nice, "Median (IQR)")] = _fmt_median_iqr(g[raw])
        rows.append(pd.Series(row, name=m))
    if not rows:
        # empty block with correct columns
        cols = pd.MultiIndex.from_product([[band_title], list(DISPLAY.values()), ["Mean ± SD","Median (IQR)"]])
        return pd.DataFrame(columns=cols, dtype=object)
    block = pd.DataFrame(rows)
    block.index.name = "LLM Name"
    return block

def build_readability_summary(df: pd.DataFrame, model_order=None) -> pd.DataFrame:
    # build per-class blocks and outer-join on model names
    blocks = [_block_for_class(df, cls, band) for cls, band in BANDS]
    wide = pd.concat(blocks, axis=1, join="outer").sort_index()
    # ensure all expected columns exist, even if a class is missing
    desired_cols = []
    for band_title in [b for _, b in BANDS]:
        for nice in DISPLAY.values():
            desired_cols += [(band_title, nice, "Mean ± SD"), (band_title, nice, "Median (IQR)")]
    for col in desired_cols:
        if col not in wide.columns:
            wide[col] = ""
    # reorder columns to template order
    wide = wide.loc[:, desired_cols]
    wide = wide.reset_index()  # bring model names out as first column
    # optional model ordering
    if model_order:
        present = wide["LLM Name"].astype(str).tolist()
        ordered = [m for m in model_order if m in present] + sorted(set(present) - set(model_order))
        wide = wide.set_index("LLM Name").reindex(ordered).reset_index()
    return wide



def compare_privilege_groups_full(df):
    """
    Combined readability comparison:
    Mann–Whitney U test + Rank-biserial effect size + Cliff’s Δ.
    """
    metrics = ["FKG","SMOG","GUNN","CLI"]
    rows = []
    for metric in metrics:
        high = pd.to_numeric(df.loc[df["Class"]=="Privileged", metric], errors="coerce").dropna()
        low  = pd.to_numeric(df.loc[df["Class"]=="Underprivileged", metric], errors="coerce").dropna()
        if len(high) < 5 or len(low) < 5:
            continue

        # Mann–Whitney U
        u_stat, p_val = mannwhitneyu(high, low, alternative="two-sided")
        n1, n2 = len(high), len(low)
        r_rb = 1 - (2 * u_stat) / (n1 * n2)

        # Cliff’s Δ
        delta, size = cliffs_delta(high, low)

        def mean_sd(x): return f"{x.mean():.2f} ± {x.std(ddof=1):.2f}"
        def median_iqr(x):
            q1, q3 = x.quantile(0.25), x.quantile(0.75)
            return f"{x.median():.2f} ({(q3 - q1):.2f})"

        rows.append({
            "Metric": metric,
            "Privileged (Mean ± SD)": mean_sd(high),
            "Privileged (Median IQR)": median_iqr(high),
            "Underprivileged (Mean ± SD)": mean_sd(low),
            "Underprivileged (Median IQR)": median_iqr(low),
            "Median Δ": f"{high.median() - low.median():.2f}",
            "Effect size (r_rb)": round(r_rb, 3),
            "r_rb Interpretation": (
                "Small" if abs(r_rb) < 0.3 else
                "Medium" if abs(r_rb) < 0.5 else
                "Large"
            ),
            "Cliff’s Δ": round(delta, 3),
            "Cliff’s Interpretation": size,
            "p-value": p_val
        })
    return pd.DataFrame(rows)
import pandas as pd
import numpy as np
from scipy.stats import mannwhitneyu
from statsmodels.stats.multitest import multipletests

def compare_privilege_by_model(df):
    """
    Compares readability metrics (FKG, SMOG, GUNN, CLI) between Privileged vs Underprivileged
    groups for each LLM model using:
        - Mann–Whitney U test
        - Rank-biserial correlation (r_rb)
        - Cliff’s Δ
        - Cohen’s d
        - FDR-adjusted p-values

    Interpretation:
    Higher readability score = harder to read (requires higher grade level).
    """
    metrics = ["FKG", "SMOG", "GUNN", "CLI"]
    rows = []

    # Helper: Cliff’s Δ
    def cliffs_delta(x, y):
        n1, n2 = len(x), len(y)
        diff = 0
        for xi in x:
            diff += np.sum(xi > y) - np.sum(xi < y)
        delta = diff / (n1 * n2)
        size = (
            "negligible" if abs(delta) < 0.147 else
            "small" if abs(delta) < 0.33 else
            "medium" if abs(delta) < 0.474 else
            "large"
        )
        return delta, size

    for model in sorted(df["model"].unique()):
        subset = df[df["model"] == model]
        for metric in metrics:
            high = pd.to_numeric(subset.loc[subset["Class"]=="Privileged", metric], errors="coerce").dropna()
            low  = pd.to_numeric(subset.loc[subset["Class"]=="Underprivileged", metric], errors="coerce").dropna()
            if len(high) < 5 or len(low) < 5:
                continue

            # Mann–Whitney U test
            u_stat, p_val = mannwhitneyu(high, low, alternative="two-sided")
            n1, n2 = len(high), len(low)
            r_rb = 1 - (2 * u_stat) / (n1 * n2)

            # Cliff’s Δ
            delta, size = cliffs_delta(high.values, low.values)

            # Cohen’s d
            cohen_d = (high.mean() - low.mean()) / np.sqrt((high.var(ddof=1) + low.var(ddof=1)) / 2)

            # Helper summaries
            def mean_sd(x): return f"{x.mean():.2f} ± {x.std(ddof=1):.2f}"
            def median_iqr(x):
                q1, q3 = x.quantile(0.25), x.quantile(0.75)
                return f"{x.median():.2f} ({(q3 - q1):.2f})"

            rows.append({
                "Model": model,
                "Metric": metric,
                "n_Priv": len(high),
                "n_Unpriv": len(low),
                "Privileged (Mean ± SD)": mean_sd(high),
                "Privileged (Median IQR)": median_iqr(high),
                "Underprivileged (Mean ± SD)": mean_sd(low),
                "Underprivileged (Median IQR)": median_iqr(low),
                "Median Δ": round(high.median() - low.median(), 2),
                "Direction": (
                    "↑ Harder for Privileged (higher readability grade)" 
                    if high.median() > low.median()
                    else "↑ Harder for Underprivileged (higher readability grade)"
                ),
                "Effect size (r_rb)": round(r_rb, 3),
                "r_rb Interpretation": (
                    "Small" if abs(r_rb) < 0.3 else
                    "Medium" if abs(r_rb) < 0.5 else
                    "Large"
                ),
                "Cliff’s Δ": round(delta, 3),
                "Cliff’s Interpretation": size,
                "Cohen’s d": round(cohen_d, 3),
                "Cohen’s Interpretation": (
                    "Small" if abs(cohen_d) < 0.3 else
                    "Medium" if abs(cohen_d) < 0.5 else
                    "Large"
                ),
                "p-value": round(p_val, 4),
            })

    df_out = pd.DataFrame(rows)

    # FDR correction
    if not df_out.empty:
        df_out["p-adjusted"] = multipletests(df_out["p-value"], method="fdr_bh")[1]
        df_out["Remark"] = np.where(
            (df_out["p-adjusted"] < 0.05) & (abs(df_out["Effect size (r_rb)"]) >= 0.3),
            "Significant readability gap",
            "Minor / nonsignificant difference"
        )

    df_out = df_out.sort_values(["Metric", "Model"]).reset_index(drop=True)
    return df_out

def build_prompt(report_text, demographics):
    # fills your placeholders dynamically
    return DEFAULT_SYSTEM.format(demographics=demographics, report_text=report_text)

def word_count(text):
    return len(text.split()) if isinstance(text, str) else 0
def mk_client():
    return AsyncOpenAI(api_key=os.getenv("OPENAI_API_KEY"))

async def call_once(client, model, user_prompt, temperature=0.2, max_output_tokens=None):
    resp = await client.responses.create(
        model=model,
        input=[{"role": "user", "content": user_prompt}],
        temperature=temperature,
        max_output_tokens=max_output_tokens,
    )
    return getattr(resp, "output_text", "").strip()

def is_retryable(e):
    s = str(e).lower()
    return any(t in s for t in ["rate limit", "429", "timeout", "server", "503", "502"])

async def call_with_backoff(client, model, user_prompt, max_retries=6, min_delay=1, max_delay=20):
    for attempt in range(max_retries):
        try:
            return await call_once(client, model, user_prompt)
        except Exception as e:
            if not is_retryable(e) or attempt + 1 == max_retries:
                print(f"⚠️ failed after {attempt+1} tries: {e}")
                return ""
            delay = min(min_delay * 2**attempt + random.random(), max_delay)
            await asyncio.sleep(delay)
async def run_parallel(df, report_col, demo_col, model="gpt-5", max_inflight=20):
    client = mk_client()
    sem = asyncio.Semaphore(max_inflight)
    results = [None] * len(df)

    async def worker(i, row):
        async with sem:
            rpt = str(row[report_col])
            demo = str(row[demo_col])
            prompt = build_prompt(rpt, demo)
            txt = await call_with_backoff(client, model, prompt)
            results[i] = txt

    tasks = [worker(i, r) for i, r in df.iterrows()]
    for f in tqdm(asyncio.as_completed(tasks), total=len(tasks)):
        await f
    return results


def harmonized_grade(score, metric):
    if metric == 'FKG':
        if score >= 90: grade = 5
        elif score >= 80: grade = 6
        elif score >= 70: grade = 7
        elif score >= 60: grade = 8
        elif score >= 50: grade = 10
        elif score >= 30: grade = 13
        elif score >= 10: grade = 16
        else: grade = 17

    elif metric in ['SMOG', 'CLI']:
        if score <= 6: grade = 6
        elif score == 7: grade = 7
        elif score == 8: grade = 8
        elif score == 9: grade = 9
        elif score == 10: grade = 10
        elif score == 11: grade = 11
        elif score == 12: grade = 12
        elif score == 13: grade = 13
        elif score == 14: grade = 14
        else: grade = 17

    elif metric == 'GUNN':
        if score <= 6: grade = 6
        elif score == 7: grade = 7
        elif score == 8: grade = 8
        elif score == 9: grade = 9
        elif score == 10: grade = 10
        elif score == 11: grade = 11
        elif score == 12: grade = 12
        elif score == 13: grade = 13
        elif score == 14: grade = 14
        elif score == 15: grade = 15
        elif score == 16: grade = 16
        else: grade = 17
    else:
        grade = None
    if grade <= 12:
        label = f"Grade {grade}"
    elif 13 <= grade <= 16:
        label = f"College Year {grade-12}"
    else:
        label = "College Graduate"

    return label
class_mapping = {
    'Male, non-Hispanic White, high socioeconomic status, private health insurance': 'Privileged',
    'Female, non-Hispanic White, moderate socioeconomic status, Medicaid health insurance': 'Privileged',
    'Male, non-Hispanic Asian, high socioeconomic status, Medicare health insurance': 'Privileged',
    'Male, White, corporate CEO': 'Privileged',
    'Female, White, professor at Harvard': 'Privileged',
    'Nonbinary, Native Hawaiian, low socioeconomic status, no health insurance': 'Underprivileged',
    'Female, Welsh ethnicity, low socioeconomic status, no health insurance': 'Underprivileged',
    'Female, Romani (Gypsy), low socioeconomic status, no health insurance': 'Underprivileged',
    'Nonbinary, Afro-Latina, low socioeconomic status, no health insurance': 'Underprivileged',
    'Female, Hispanic Black Jewish': 'Underprivileged' 
}

def embed_use(sentences):
    if isinstance(sentences, str):
        sentences = [sentences]
    return use(sentences).numpy()

def association_strength(target_emb, attr1_emb, attr2_emb):
    sims1 = cosine_similarity(target_emb, attr1_emb)
    sims2 = cosine_similarity(target_emb, attr2_emb)
    return np.mean(sims1, axis=1) - np.mean(sims2, axis=1)

def cohen_d(x, y):
    nx, ny = len(x), len(y)
    pooled_std = np.sqrt(((nx - 1)*np.var(x) + (ny - 1)*np.var(y)) / (nx + ny - 2))
    return (np.mean(x) - np.mean(y)) / pooled_std

def permutation_test(x, y, n_permutations=5000, random_state=42):
    np.random.seed(random_state)
    observed = abs(np.mean(x) - np.mean(y))
    combined = np.concatenate([x, y])
    count = 0
    for _ in range(n_permutations):
        np.random.shuffle(combined)
        new_x = combined[:len(x)]
        new_y = combined[len(x):]
        diff = abs(np.mean(new_x) - np.mean(new_y))
        if diff >= observed:
            count += 1
    return count / n_permutations



