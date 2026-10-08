
import math
import random
import customtkinter as ctk
import tkinter as tk
from tkinter import messagebox


class BombDrawApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("抽炸彈遊戲")
        self.geometry("900x700")
        self.minsize(800, 600)

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        # =========================
        # 遊戲設定
        # =========================
        self.bomb_probability = 0.20
        self.avoid_probability = 0.80
        self.immunity_count = 1

        # =========================
        # 本局狀態
        # =========================
        self.draw_count = 0
        self.bomb_count = 0
        self.used_immunity = 0
        self.game_over = False

        # =========================
        # 總統計
        # =========================
        self.total_games = 0
        self.total_deaths = 0
        self.total_draws = 0

        self.create_ui()
        self.reset_game(log_message=False)

    # =========================================================
    # GUI
    # =========================================================
    def create_ui(self):

        # -------------------------
        # Title
        # -------------------------
        title = ctk.CTkLabel(
            self,
            text="抽炸彈",
            font=ctk.CTkFont(size=30, weight="bold")
        )
        title.pack(pady=(20, 10))

        subtitle = ctk.CTkLabel(
            self,
            text="抽到炸彈且免死次數已用完，即判定爆炸",
            font=ctk.CTkFont(size=14)
        )
        subtitle.pack(pady=(0, 15))

        # -------------------------
        # 設定區
        # -------------------------
        setting_frame = ctk.CTkFrame(self)
        setting_frame.pack(fill="x", padx=30, pady=10)

        setting_title = ctk.CTkLabel(
            setting_frame,
            text="遊戲設定",
            font=ctk.CTkFont(size=18, weight="bold")
        )
        setting_title.grid(
            row=0,
            column=0,
            columnspan=6,
            padx=15,
            pady=(15, 10),
            sticky="w"
        )

        # 炸彈機率
        bomb_label = ctk.CTkLabel(
            setting_frame,
            text="炸彈機率 (%)"
        )
        bomb_label.grid(
            row=1,
            column=0,
            padx=(15, 5),
            pady=10
        )

        self.bomb_entry = ctk.CTkEntry(
            setting_frame,
            width=100,
            placeholder_text="20"
        )
        self.bomb_entry.grid(
            row=1,
            column=1,
            padx=5,
            pady=10
        )

        self.bomb_entry.insert(0, "20")

        # 避開機率
        avoid_label = ctk.CTkLabel(
            setting_frame,
            text="避開炸彈 (%)"
        )
        avoid_label.grid(
            row=1,
            column=2,
            padx=(15, 5),
            pady=10
        )

        self.avoid_value_label = ctk.CTkLabel(
            setting_frame,
            text="80.00%",
            width=80
        )
        self.avoid_value_label.grid(
            row=1,
            column=3,
            padx=5,
            pady=10
        )

        # 免死
        immunity_label = ctk.CTkLabel(
            setting_frame,
            text="免死次數"
        )
        immunity_label.grid(
            row=1,
            column=4,
            padx=(15, 5),
            pady=10
        )

        self.immunity_entry = ctk.CTkEntry(
            setting_frame,
            width=100,
            placeholder_text="1"
        )
        self.immunity_entry.grid(
            row=1,
            column=5,
            padx=(5, 15),
            pady=10
        )

        self.immunity_entry.insert(0, "1")

        # 套用設定
        apply_button = ctk.CTkButton(
            setting_frame,
            text="套用設定",
            command=self.apply_settings
        )
        apply_button.grid(
            row=2,
            column=0,
            columnspan=6,
            padx=15,
            pady=(0, 15)
        )

        # -------------------------
        # 狀態資訊
        # -------------------------
        status_frame = ctk.CTkFrame(self)
        status_frame.pack(
            fill="x",
            padx=30,
            pady=10
        )

        self.status_label = ctk.CTkLabel(
            status_frame,
            text="準備開始",
            font=ctk.CTkFont(size=20, weight="bold")
        )
        self.status_label.pack(pady=(15, 5))

        self.stat_label = ctk.CTkLabel(
            status_frame,
            text="",
            font=ctk.CTkFont(size=14)
        )
        self.stat_label.pack(pady=(0, 15))

        # -------------------------
        # 按鈕
        # -------------------------
        button_frame = ctk.CTkFrame(
            self,
            fg_color="transparent"
        )
        button_frame.pack(
            fill="x",
            padx=30,
            pady=10
        )

        self.draw_button = ctk.CTkButton(
            button_frame,
            text="抽　獎",
            height=60,
            font=ctk.CTkFont(
                size=22,
                weight="bold"
            ),
            command=self.draw
        )
        self.draw_button.pack(
            side="left",
            expand=True,
            fill="x",
            padx=(0, 10)
        )

        self.reset_button = ctk.CTkButton(
            button_frame,
            text="Reset",
            height=60,
            font=ctk.CTkFont(
                size=20,
                weight="bold"
            ),
            command=self.reset_game
        )
        self.reset_button.pack(
            side="left",
            expand=True,
            fill="x",
            padx=(10, 0)
        )

        # -------------------------
        # 狀態訊息
        # -------------------------
        log_title = ctk.CTkLabel(
            self,
            text="狀態訊息",
            font=ctk.CTkFont(
                size=18,
                weight="bold"
            )
        )
        log_title.pack(
            anchor="w",
            padx=35,
            pady=(10, 5)
        )

        text_frame = ctk.CTkFrame(self)
        text_frame.pack(
            fill="both",
            expand=True,
            padx=30,
            pady=(0, 10)
        )

        self.log_text = tk.Text(
            text_frame,
            bg="#1e1e1e",
            fg="#eeeeee",
            insertbackground="#ffffff",
            font=("Consolas", 11),
            wrap="word",
            bd=0,
            padx=12,
            pady=12
        )
        self.log_text.pack(
            side="left",
            fill="both",
            expand=True
        )

        scrollbar = ctk.CTkScrollbar(
            text_frame,
            command=self.log_text.yview
        )
        scrollbar.pack(
            side="right",
            fill="y"
        )

        self.log_text.configure(
            yscrollcommand=scrollbar.set
        )

        # Text 標籤顏色
        self.log_text.tag_configure(
            "normal",
            foreground="#eeeeee"
        )

        self.log_text.tag_configure(
            "safe",
            foreground="#66cc66"
        )

        self.log_text.tag_configure(
            "bomb",
            foreground="#ffcc66"
        )

        self.log_text.tag_configure(
            "dead",
            foreground="#ff5555"
        )

        self.log_text.tag_configure(
            "stat",
            foreground="#66b3ff"
        )

        # -------------------------
        # 清除總統計
        # -------------------------
        clear_button = ctk.CTkButton(
            self,
            text="清除總統計",
            width=120,
            height=30,
            command=self.clear_total_statistics
        )
        clear_button.pack(
            pady=(0, 15)
        )

    # =========================================================
    # 套用設定
    # =========================================================
    def apply_settings(self):

        try:
            bomb_percent = float(
                self.bomb_entry.get()
            )

            immunity = int(
                self.immunity_entry.get()
            )

            if not 0 <= bomb_percent <= 100:
                raise ValueError

            if immunity < 0:
                raise ValueError

        except ValueError:
            messagebox.showerror(
                "設定錯誤",
                "炸彈機率必須介於 0～100，"
                "免死次數必須是 0 以上的整數。"
            )
            return

        self.bomb_probability = bomb_percent / 100
        self.avoid_probability = 1 - self.bomb_probability
        self.immunity_count = immunity

        self.avoid_value_label.configure(
            text=f"{self.avoid_probability * 100:.2f}%"
        )

        self.reset_game()

        self.add_log(
            "設定已套用\n"
            f"炸彈機率：{self.bomb_probability * 100:.2f}%\n"
            f"避開炸彈機率：{self.avoid_probability * 100:.2f}%\n"
            f"免死次數：{self.immunity_count}\n",
            "stat"
        )

    # =========================================================
    # 抽獎
    # =========================================================
    # =========================================================
    # 抽獎
    # =========================================================
    def draw(self):

        if self.game_over:
            self.add_log(
                "本局已結束，請按 Reset 重新開始。\n",
                "dead"
            )
            return

        self.draw_count += 1
        self.total_draws += 1

        # 是否抽中炸彈
        is_bomb = (
            random.random()
            < self.bomb_probability
        )

        # =====================================================
        # 抽到炸彈
        # =====================================================
        if is_bomb:

            self.bomb_count += 1

            # 還有免死
            if self.used_immunity < self.immunity_count:

                self.used_immunity += 1

                remaining_immunity = (
                    self.immunity_count
                    - self.used_immunity
                )

                next_draw = self.draw_count + 1
                
                # 截至下一抽的累積爆炸機率 與 累積安全機率
                next_cumulative_probability = self.calculate_death_by_draw(next_draw)
                next_safe_probability = 1.0 - next_cumulative_probability

                self.add_log(
                    f"第 {self.draw_count} 抽：抽到炸彈！(使用免死，目前仍存活)\n"
                    f"下一抽，第 {next_draw} 抽的理論累積爆炸機率：{next_cumulative_probability * 100:.2f}%，安全機率：{next_safe_probability * 100:.2f}%\n",
                    "bomb"
                )

                self.status_label.configure(
                    text=f"抽到炸彈，免死剩餘 {remaining_immunity} 次"
                )

            # 沒有免死 → 死亡
            else:

                self.game_over = True

                self.total_games += 1
                self.total_deaths += 1

                exact_probability = (
                    self.calculate_death_on_draw(
                        self.draw_count
                    )
                )

                cumulative_probability = (
                    self.calculate_death_by_draw(
                        self.draw_count
                    )
                )

                one_in = (
                    1 / cumulative_probability
                    if cumulative_probability > 0
                    else 0
                )

                self.add_log(
                    f"第 {self.draw_count} 抽：抽到炸彈！\n"
                    f"免死次數已全部用完。\n"
                    f"結果：爆炸，遊戲結束。\n\n"
                    f"本抽爆炸的統計機率："
                    f"{exact_probability * 100:.2f}%\n"
                    f"第 {self.draw_count} 抽的爆炸概率："
                    f"{cumulative_probability * 100:.2f}%\n",
                    "dead"
                )

                self.status_label.configure(
                    text="爆炸！你輸了"
                )

                self.draw_button.configure(
                    state="disabled"
                )

        # =====================================================
        # 安全
        # =====================================================
        else:

            next_draw = self.draw_count + 1

            # 截至下一抽的累積爆炸機率
            next_cumulative_probability = (
                self.calculate_death_by_draw(
                    next_draw
                )
            )
            
            # 截至下一抽的累積安全機率 (1 - 累積爆炸機率)
            next_safe_probability = 1.0 - next_cumulative_probability

            self.add_log(
                f"第 {self.draw_count} 抽：安全，目前仍存活。\n"
                f"下一抽統計爆炸概率：{next_cumulative_probability * 100:.2f}%，安全概率：{next_safe_probability * 100:.2f}%\n",
                "safe"
            )

            self.status_label.configure(
                text="安全"
            )

        self.update_statistics()

    # =========================================================
    # 計算「第 N 抽死亡」
    #
    # 假設免死 m 次：
    #
    # 前 N-1 抽必須出現剛好 m 顆炸彈
    # 第 N 抽再抽到炸彈
    #
    # C(N-1,m) * p^(m+1) * (1-p)^(N-m-1)
    # =========================================================
    def calculate_death_on_draw(self, n):

        m = self.immunity_count
        p = self.bomb_probability
        q = 1 - p

        if n <= m:
            return 0.0

        return (
            math.comb(n - 1, m)
            * (p ** (m + 1))
            * (q ** (n - m - 1))
        )

    # =========================================================
    # 計算「N 抽以內死亡」
    #
    # 至少出現 m+1 顆炸彈才會死亡
    # =========================================================
    def calculate_death_by_draw(self, n):

        m = self.immunity_count
        p = self.bomb_probability
        q = 1 - p

        if n <= m:
            return 0.0

        probability = 0.0

        for k in range(m + 1, n + 1):
            probability += (
                math.comb(n, k)
                * (p ** k)
                * (q ** (n - k))
            )

        return probability

    # =========================================================
    # 更新統計
    # =========================================================
    def update_statistics(self):

        cumulative_probability = (
            self.calculate_death_by_draw(
                self.draw_count
            )
        )

        if self.total_games > 0:
            actual_death_rate = (
                self.total_deaths
                / self.total_games
            )
        else:
            actual_death_rate = 0.0

        remaining_immunity = max(
            0,
            self.immunity_count
            - self.used_immunity
        )

        self.stat_label.configure(
            text=(
                f"本局：{self.draw_count} 抽   "
                f"炸彈：{self.bomb_count}   "
                f"免死剩餘：{remaining_immunity}\n"
                f"下一抽爆炸概率："
                f"{cumulative_probability * 100:.2f}%"
            )
        )

    # =========================================================
    # Reset 本局
    # =========================================================
    def reset_game(self, log_message=True):

        self.draw_count = 0
        self.bomb_count = 0
        self.used_immunity = 0
        self.game_over = False

        self.draw_button.configure(
            state="normal"
        )

        self.status_label.configure(
            text="準備開始"
        )

        self.update_statistics()

        if log_message:
            self.add_log(
                "\n"
                "==============================\n"
                "新的一局開始\n"
                f"炸彈機率："
                f"{self.bomb_probability * 100:.2f}%\n"
                f"避開炸彈機率："
                f"{self.avoid_probability * 100:.2f}%\n"
                f"免死次數："
                f"{self.immunity_count}\n"
                "==============================\n",
                "stat"
            )

    # =========================================================
    # 清除總統計
    # =========================================================
    def clear_total_statistics(self):

        self.total_games = 0
        self.total_deaths = 0
        self.total_draws = 0

        self.update_statistics()

        self.add_log(
            "總統計已清除。\n",
            "stat"
        )

    # =========================================================
    # 寫入狀態訊息
    # =========================================================
    def add_log(self, message, tag="normal"):

        self.log_text.insert(
            tk.END,
            message + "\n",
            tag
        )

        self.log_text.see(tk.END)


# =============================================================
# 啟動程式
# =============================================================

if __name__ == "__main__":
    app = BombDrawApp()
    app.mainloop()