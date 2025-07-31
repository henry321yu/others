def number_to_column_name(n):
    result = ""
    while n > 0:
        n, remainder = divmod(n - 1, 26)
        result = chr(65 + remainder) + result
    return result

def main():
    try:
        col = int(input("請輸入欄號（column number）："))
        if col <= 0 :
            print("欄號與列號必須為正整數！")
            return
        col_name = number_to_column_name(col)
        print(f"Excel 儲存格名稱為：{col_name}")
    except ValueError:
        print("請輸入有效的整數！")

if __name__ == "__main__":
    main()
