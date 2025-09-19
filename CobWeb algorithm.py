# Fixed-point iteration & cobweb plots (matplotlib만 사용, 색상 지정 없음)
import numpy as np
import matplotlib.pyplot as plt
from math import sqrt, cos

def fixed_point_iterate(f, x0, tol=1e-8, max_iter=100, alpha=1.0):
    """
    고정점 반복 (완화계수 alpha 포함).
    반환: {history, root_estimate, iters, ...}
    """
    hist = [float(x0)]
    x = float(x0)
    for n in range(max_iter):
        x_next = (1 - alpha) * x + alpha * f(x)
        hist.append(float(x_next))
        if abs(x_next - x) < tol:
            break
        x = x_next
    return {
        "x0": x0,
        "alpha": alpha,
        "tol": tol,
        "max_iter": max_iter,
        "iters": len(hist) - 1,
        "root_estimate": hist[-1],
        "history": hist,
    }

def cobweb_data(f, x0, steps):
    """
    반복점 x_n 시퀀스와 (n, x_n, f(x_n)) 표 생성하기
    """
    import pandas as pd
    xs = [float(x0)]
    for _ in range(steps):
        xs.append(float(f(xs[-1])))
    rows = [{"n": n, "x_n": xs[n], "f(x_n)": f(xs[n])} for n in range(len(xs)-1)]
    return xs, pd.DataFrame(rows)

def plot_cobweb(f, xs, x_range, num_curve_points=800, title="Cobweb diagram"):
    """
    y=f(x), y=x, 코브웹 경로(수평→수직) 1개 그림(단일 플롯).
    """
    xmin, xmax = x_range
    grid = np.linspace(xmin, xmax, num_curve_points)
    plt.figure(figsize=(6,6))
    plt.plot(grid, [f(x) for x in grid], label="y = f(x)")
    plt.plot(grid, grid, label="y = x")
    for n in range(len(xs) - 1):
        x_n = xs[n]
        f_xn = f(x_n)
        plt.plot([x_n, f_xn], [f_xn, f_xn])        # 수평
        if n < len(xs) - 2:
            ffxn = f(f_xn)
            plt.plot([f_xn, f_xn], [f_xn, ffxn])   # 수직
    plt.title(title)
    plt.xlabel("x"); plt.ylabel("y")
    plt.legend()
    plt.axis("equal")
    plt.xlim(xmin, xmax); plt.ylim(xmin, xmax)
    plt.tight_layout()

# ---------- ex1: f(x) = sqrt(3 + x) ----------
def f_sqrt3x(x): return sqrt(3.0 + x)
res1 = fixed_point_iterate(f_sqrt3x, x0=1.0, tol=1e-10, max_iter=100, alpha=1.0)
xs1, df1 = cobweb_data(f_sqrt3x, x0=1.0, steps=12)
print("sqrt(3+x) 추정 고정점:", res1["root_estimate"], "반복수:", res1["iters"])
plot_cobweb(f_sqrt3x, xs1, x_range=(0.0, 3.0), title="Cobweb: f(x)=sqrt(3+x), x0=1.0")
plt.show()

# ---------- ex2: f(x) = cos(x) ----------
def f_cos(x): return cos(x)
res2 = fixed_point_iterate(f_cos, x0=0.5, tol=1e-12, max_iter=200, alpha=1.0)
xs2, df2 = cobweb_data(f_cos, x0=0.5, steps=15)
print("cos(x) 도티수 근사:", res2["root_estimate"], "반복수:", res2["iters"])
plot_cobweb(f_cos, xs2, x_range=(0.0, 1.0), title="Cobweb: f(x)=cos(x), x0=0.5")
plt.show()
