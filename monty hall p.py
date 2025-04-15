import tkinter as tk
from tkinter import messagebox
import random

class MontyHallGame:
    def __init__(self, root):
        self.root = root
        self.root.title("Monty Hall 模擬遊戲")

        self.num_doors = 3
        self.car_door = None
        self.guest_choice = None
        self.revealed_doors = []
        self.doors = []

        self.switch_button = None
        self.keep_button = None

        self.setup_ui()

    def setup_ui(self):
        self.top_frame = tk.Frame(self.root)
        self.top_frame.pack(pady=10)

        tk.Label(self.top_frame, text="設定門的數量: ").pack(side=tk.LEFT)
        self.door_entry = tk.Entry(self.top_frame, width=5)
        self.door_entry.insert(0, "3")
        self.door_entry.pack(side=tk.LEFT)
        tk.Button(self.top_frame, text="建立遊戲", command=self.setup_game).pack(side=tk.LEFT)

        self.info_label = tk.Label(self.root, text="請設定遊戲")
        self.info_label.pack(pady=10)

        self.door_frame = tk.Frame(self.root)
        self.door_frame.pack()

    def setup_game(self):
        try:
            self.num_doors = int(self.door_entry.get())
            if self.num_doors < 3:
                raise ValueError
        except ValueError:
            messagebox.showerror("錯誤", "門的數量請輸入 3 或以上")
            return

        for widget in self.door_frame.winfo_children():
            widget.destroy()

        if self.switch_button:
            self.switch_button.destroy()
        if self.keep_button:
            self.keep_button.destroy()

        self.doors = []
        self.car_door = random.randint(0, self.num_doors - 1)
        self.guest_choice = None
        self.revealed_doors = []

        self.info_label.config(text="主持人已選擇一扇門藏車，請來賓選擇")

        for i in range(self.num_doors):
            btn = tk.Button(self.door_frame, text=f"門 {i+1}\n機率: 1/{self.num_doors}", width=12, height=4,
                            command=lambda i=i: self.guest_select_door(i))
            btn.grid(row=0, column=i, padx=5, pady=5)
            self.doors.append(btn)

    def guest_select_door(self, index):
        if self.guest_choice is not None:
            return

        self.guest_choice = index
        self.info_label.config(text=f"來賓選擇了門 {index+1}，主持人開門中...")

        self.reveal_goat_doors()
        self.update_probabilities()

    def reveal_goat_doors(self):
        remaining = [i for i in range(self.num_doors) if i != self.guest_choice and i != self.car_door]
        if self.num_doors == 3:
            to_reveal = random.choice(remaining)
            self.revealed_doors = [to_reveal]
        else:
            self.revealed_doors = remaining[:-1]  # 留下1個門+玩家選的

        for i in self.revealed_doors:
            self.doors[i].config(state=tk.DISABLED, text=f"門 {i+1}\n🐐")

        self.info_label.config(text=f"已開啟 {len(self.revealed_doors)} 扇山羊門，是否要換門？")

        self.switch_button = tk.Button(self.root, text="換門", command=self.switch_choice)
        self.switch_button.pack(pady=5)
        self.keep_button = tk.Button(self.root, text="不換門", command=self.keep_choice)
        self.keep_button.pack(pady=5)

    def update_probabilities(self):
        base_prob = 1 / self.num_doors
        remaining_doors = [i for i in range(self.num_doors) if i not in self.revealed_doors]

        for i, btn in enumerate(self.doors):
            if i in self.revealed_doors:
                continue

            if i == self.guest_choice:
                prob = base_prob
            else:
                prob = (self.num_doors - 1) / self.num_doors / (len(remaining_doors) - 1)

            btn.config(text=f"門 {i+1}\n機率: {prob:.2%}")

    def switch_choice(self):
        remaining = [i for i in range(self.num_doors) if i not in self.revealed_doors and i != self.guest_choice]
        self.guest_choice = remaining[0]  # 自動換成唯一可選門
        self.end_game()

    def keep_choice(self):
        self.end_game()

    def end_game(self):
        for i, btn in enumerate(self.doors):
            if i == self.car_door:
                btn.config(bg="gold", text=f"門 {i+1}\n🚗")
            elif i in self.revealed_doors:
                continue
            else:
                btn.config(bg="gray")

        if self.guest_choice == self.car_door:
            self.info_label.config(text="🎉 恭喜你選到了車子！")
        else:
            self.info_label.config(text="😢 可惜，是山羊。")

        if self.switch_button:
            self.switch_button.destroy()
        if self.keep_button:
            self.keep_button.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = MontyHallGame(root)
    root.mainloop()
