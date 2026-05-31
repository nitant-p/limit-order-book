#!/usr/bin/env python3
"""Generate SVG benchmark charts from a benchmark CSV."""

from __future__ import annotations

import csv
import html
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DATA_FILE = ROOT / "docs" / "benchmark_results_v1.csv"
DEFAULT_OUT_DIR = ROOT / "docs" / "benchmark_graphs"

COLORS = ["#2563eb", "#f97316", "#16a34a", "#7c3aed"]
TEXT = "#111827"
MUTED = "#6b7280"
GRID = "#e5e7eb"


def load_rows(data_file: Path) -> list[dict[str, str]]:
    with data_file.open(newline="") as f:
        return list(csv.DictReader(f))


def fmt_value(value: float, suffix: str) -> str:
    if value >= 100:
        return f"{value:.0f}{suffix}"
    if value >= 10:
        return f"{value:.1f}{suffix}"
    return f"{value:.2f}{suffix}"


def svg_text(x: float, y: float, text: str, size: int = 13, fill: str = TEXT, anchor: str = "start") -> str:
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" font-family="Inter, Arial, sans-serif" '
        f'font-size="{size}" fill="{fill}" text-anchor="{anchor}">{html.escape(text)}</text>'
    )


def grouped_bar_chart(
    output: Path,
    title: str,
    subtitle: str,
    labels: list[str],
    series: list[tuple[str, list[float]]],
    unit_suffix: str,
) -> None:
    width = 1180
    row_h = 48
    top = 92
    left = 300
    right = 130
    bottom = 46
    height = top + row_h * len(labels) + bottom
    plot_w = width - left - right
    max_value = max(max(values) for _, values in series)
    tick_count = 4
    tick_step = max_value / tick_count if max_value else 1

    out: list[str] = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        svg_text(28, 34, title, 22),
        svg_text(28, 58, subtitle, 13, MUTED),
    ]

    for i in range(tick_count + 1):
        value = i * tick_step
        x = left + (value / max_value) * plot_w if max_value else left
        out.append(f'<line x1="{x:.1f}" y1="{top - 18}" x2="{x:.1f}" y2="{height - bottom + 8}" stroke="{GRID}" stroke-width="1"/>')
        out.append(svg_text(x, top - 26, fmt_value(value, unit_suffix), 11, MUTED, "middle"))

    legend_x = left
    for index, (name, _) in enumerate(series):
        x = legend_x + index * 112
        out.append(f'<rect x="{x}" y="60" width="12" height="12" fill="{COLORS[index % len(COLORS)]}" rx="2"/>')
        out.append(svg_text(x + 18, 71, name, 12, MUTED))

    bar_h = 14
    gap = 5
    for row, label in enumerate(labels):
        y_base = top + row * row_h
        out.append(svg_text(left - 18, y_base + 21, label, 12, TEXT, "end"))
        for index, (_, values) in enumerate(series):
            value = values[row]
            y = y_base + index * (bar_h + gap)
            w = (value / max_value) * plot_w if max_value else 0
            out.append(f'<rect x="{left}" y="{y}" width="{w:.1f}" height="{bar_h}" fill="{COLORS[index % len(COLORS)]}" rx="3"/>')
            out.append(svg_text(left + w + 8, y + 11, fmt_value(value, unit_suffix), 11, MUTED))

    out.append("</svg>")
    output.write_text("\n".join(out) + "\n")


def one_series_bar_chart(
    output: Path,
    title: str,
    subtitle: str,
    labels: list[str],
    values: list[float],
    unit_suffix: str,
    color: str,
) -> None:
    grouped_bar_chart(output, title, subtitle, labels, [("value", values)], unit_suffix)
    text = output.read_text()
    text = text.replace(COLORS[0], color)
    text = text.replace('<rect x="300" y="60" width="12" height="12" fill="' + color + '" rx="2"/>\n' + svg_text(318, 71, "value", 12, MUTED), "")
    output.write_text(text)


def rows_for(rows: list[dict[str, str]], layer: str, api: str | None = None, scale: str | None = None) -> list[dict[str, str]]:
    result = [row for row in rows if row["layer"] == layer]
    if api is not None:
        result = [row for row in result if row["api"] == api]
    if scale is not None:
        result = [row for row in result if row["scale"] == scale]
    return result


def by_case(rows: list[dict[str, str]], metric: str, scale: str) -> tuple[list[str], list[float]]:
    selected = [row for row in rows if row["scale"] == scale]
    labels = [f'{row["api"]}: {row["case"]}' for row in selected]
    values = [float(row[metric]) for row in selected]
    return labels, values


def main() -> None:
    data_file = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else DEFAULT_DATA_FILE
    out_dir = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else DEFAULT_OUT_DIR

    rows = load_rows(data_file)
    out_dir.mkdir(parents=True, exist_ok=True)

    engine_rows = rows_for(rows, "MatchingEngine")
    engine_labels_1k, engine_latency_1k = by_case(engine_rows, "ns_per_op", "1000")
    _, engine_latency_10k = by_case(engine_rows, "ns_per_op", "10000")
    _, engine_ops_1k = by_case(engine_rows, "ops_per_sec", "1000")
    _, engine_ops_10k = by_case(engine_rows, "ops_per_sec", "10000")

    grouped_bar_chart(
        out_dir / "engine_latency.svg",
        "MatchingEngine API latency",
        "Lower is better. Values are mean CPU ns/op.",
        engine_labels_1k,
        [("1k", engine_latency_1k), ("10k", engine_latency_10k)],
        " ns",
    )
    grouped_bar_chart(
        out_dir / "engine_throughput.svg",
        "MatchingEngine API throughput",
        "Higher is better. Values are equivalent million operations per second.",
        engine_labels_1k,
        [("1k", [v / 1_000_000 for v in engine_ops_1k]), ("10k", [v / 1_000_000 for v in engine_ops_10k])],
        "M/s",
    )

    side_rows_10k = [row for row in rows_for(rows, "OrderBookSide", scale="10000") if row["api"] != "getDepth"]
    side_labels = [f'{row["api"]}: {row["case"]}' for row in side_rows_10k]
    side_latency = [float(row["ns_per_op"]) for row in side_rows_10k]
    side_ops_m = [float(row["ops_per_sec"]) / 1_000_000 for row in side_rows_10k]
    one_series_bar_chart(
        out_dir / "side_latency_10k.svg",
        "OrderBookSide API latency at 10k operations",
        "Lower is better. Values are mean CPU ns/op.",
        side_labels,
        side_latency,
        " ns",
        "#16a34a",
    )
    one_series_bar_chart(
        out_dir / "side_throughput_10k.svg",
        "OrderBookSide API throughput at 10k operations",
        "Higher is better. Values are equivalent million operations per second.",
        side_labels,
        side_ops_m,
        "M/s",
        "#7c3aed",
    )

    depth_rows = rows_for(rows, "OrderBookSide", api="getDepth")
    depth_labels = [row["case"] for row in depth_rows]
    depth_latency = [float(row["ns_per_op"]) for row in depth_rows]
    depth_ops_k = [float(row["ops_per_sec"]) / 1_000 for row in depth_rows]
    one_series_bar_chart(
        out_dir / "depth_latency.svg",
        "OrderBookSide::getDepth latency",
        "Lower is better. Values are mean CPU ns/read.",
        depth_labels,
        depth_latency,
        " ns",
        "#f97316",
    )
    one_series_bar_chart(
        out_dir / "depth_throughput.svg",
        "OrderBookSide::getDepth read throughput",
        "Higher is better. Values are thousand reads per second.",
        depth_labels,
        depth_ops_k,
        "k/s",
        "#2563eb",
    )


if __name__ == "__main__":
    main()
