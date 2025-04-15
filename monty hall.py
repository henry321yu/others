import random

def simulate_monty_hall(num_trials=100000):
    switch_wins = 0
    stay_wins = 0

    for _ in range(num_trials):
        # 設定車子在哪一扇門（0、1、2）
        car_position = random.randint(0, 2)
        
        # 玩家第一次選擇
        player_choice = random.randint(0, 2)
        
        # 主持人打開一扇有山羊、不是玩家選的門
        remaining_doors = [door for door in range(3) if door != player_choice and door != car_position]
        monty_opens = random.choice(remaining_doors)

        # 換門的選項（排除玩家原選門 & 主持人打開的門）
        switch_choice = next(door for door in range(3) if door != player_choice and door != monty_opens)

        # 統計結果
        if switch_choice == car_position:
            switch_wins += 1
        if player_choice == car_position:
            stay_wins += 1

    print(f"不換門中獎率: {stay_wins / num_trials:.2%}")
    print(f"換門中獎率:   {switch_wins / num_trials:.2%}")

simulate_monty_hall()
