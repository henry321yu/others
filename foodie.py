import math

def estimate_cancer_risk_taiwan(food_grams, freq_per_week, years,
                                 sodium_mg_per_meal, contains_processed_meat, plastic_heating):
    print("======= 致癌風險估算模型 =======")
    print(f"🍱	微波食品每次攝取量：{food_grams} g")
    print(f"📆	每週吃幾次		：{freq_per_week} 次")
    print(f"📅	吃的年數			：{years} 年")
    print(f"🧂	每餐鈉含量		：{sodium_mg_per_meal} mg")
    print(f"🥓	是否含加工肉		：{'是' if contains_processed_meat else '否'}")
    print(f"♨️	是否塑膠容器加熱	：{'是' if plastic_heating else '否'}")

    # === 模型參數 ===
    ref_grams = 50      # 每天50g加工肉 → 18%風險
    ref_risk = 18

    # 基本攝取量風險計算
    gram_ratio = food_grams * freq_per_week / 7 / ref_grams
    base_risk = gram_ratio * math.log(years + 1) * ref_risk
    print(f"\n📈 基礎加工肉風險增加	：約 {base_risk:.1f} %")

    # 鈉含量風險
    sodium_extra = 0
    if sodium_mg_per_meal > 800:
        sodium_extra = ((sodium_mg_per_meal - 800) // 400) * 5 * freq_per_week * years / 52
        print(f"🧂 高鈉額外風險增加	：約 {sodium_extra:.1f} %")
    else:
        print(f"🧂 鈉含量正常，不增加風險")

    # 加工肉風險（額外）
    meat_extra = 10 if contains_processed_meat else 0
    print(f"🥓 加工肉附加風險	：約 {meat_extra} %")

    # 塑膠加熱風險（假設為 base_risk 的 25%）
    plastic_extra = 0.25 * base_risk if plastic_heating else 0
    print(f"♨️ 塑膠加熱附加風險	：約 {plastic_extra:.1f} %")

    # 加總所有風險
    total = base_risk + sodium_extra + meat_extra + plastic_extra
    print(f"\n✅ 總致癌風險增加（相對）：約 {total:.1f} %")
    print("==================================")

# 🔍 範例：每週1次 500g、吃15年、每餐鈉1500mg、有加工肉、塑膠容器加熱
estimate_cancer_risk_taiwan(
    food_grams=500,
    freq_per_week=1,
    years=10,
    sodium_mg_per_meal=1500,
    contains_processed_meat=True,
    plastic_heating=True
)
