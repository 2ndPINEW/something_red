#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial
import serial.tools.list_ports
import threading
import time
import random
import os
import sys
from typing import List, Optional

# LEDマトリックスのサイズ定数
LED_TOTAL_WIDTH = 108
LED_TOTAL_HEIGHT = 36

class LEDMatrixController:
    def __init__(self, root):
        self.root = root
        self.root.title("LED Matrix Controller")
        self.root.geometry("600x500")
        self.root.resizable(True, True)
        
        # シリアル接続の状態
        self.serial_connection = None
        self.is_connected = False
        
        # UIの作成
        self.create_ui()
        
        # 初期化時にポート一覧を取得
        self.refresh_ports()

    def create_ui(self):
        """UIコンポーネントの作成"""
        # メインフレーム
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # タイトル
        title_label = ttk.Label(main_frame, text="LED Matrix Controller", font=("Arial", 16, "bold"))
        title_label.pack(pady=10)
        
        # 接続セクション
        connection_frame = ttk.LabelFrame(main_frame, text="接続設定", padding="10")
        connection_frame.pack(fill=tk.X, pady=5)
        
        # ポート選択
        port_frame = ttk.Frame(connection_frame)
        port_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(port_frame, text="シリアルポート:").pack(side=tk.LEFT, padx=5)
        self.port_combobox = ttk.Combobox(port_frame, width=30)
        self.port_combobox.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        refresh_button = ttk.Button(port_frame, text="更新", command=self.refresh_ports)
        refresh_button.pack(side=tk.LEFT, padx=5)
        
        # ボーレート選択
        baud_frame = ttk.Frame(connection_frame)
        baud_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(baud_frame, text="ボーレート:").pack(side=tk.LEFT, padx=5)
        self.baud_combobox = ttk.Combobox(baud_frame, width=15, values=["9600", "19200", "38400", "57600", "115200"])
        self.baud_combobox.pack(side=tk.LEFT, padx=5)
        self.baud_combobox.set("115200")  # デフォルト値
        
        # 接続ボタン
        self.connect_button = ttk.Button(connection_frame, text="接続", command=self.toggle_connection)
        self.connect_button.pack(pady=10)
        
        # アニメーション選択セクション
        animation_frame = ttk.LabelFrame(main_frame, text="アニメーション設定", padding="10")
        animation_frame.pack(fill=tk.X, pady=5)
        
        # アニメーションタイプ選択
        ttk.Label(animation_frame, text="アニメーションタイプ:").pack(anchor=tk.W, pady=5)
        
        self.animation_type = tk.StringVar(value="sample")
        
        animations = [
            ("サンプル（チェック柄）", "sample"),
            ("ランダム", "random"),
            ("波紋", "wave"),
        ]
        
        for text, value in animations:
            ttk.Radiobutton(animation_frame, text=text, value=value, variable=self.animation_type).pack(anchor=tk.W, padx=20)
        
        # 送信ボタン
        self.send_button = ttk.Button(animation_frame, text="アニメーション送信", command=self.send_animation, state=tk.DISABLED)
        self.send_button.pack(pady=10)
        
        # ログ表示エリア
        log_frame = ttk.LabelFrame(main_frame, text="ログ", padding="10")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=10)
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        # ステータスバー
        self.status_var = tk.StringVar(value="未接続")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def log(self, message):
        """ログメッセージを表示"""
        self.log_text.insert(tk.END, f"{message}\n")
        self.log_text.see(tk.END)  # 最新の行にスクロール
        print(message)  # コンソールにも出力

    def refresh_ports(self):
        """利用可能なシリアルポートを更新"""
        ports = serial.tools.list_ports.comports()
        port_list = []
        
        for port in ports:
            # VIDとPIDがNoneの場合のハンドリング
            vid_str = f"{port.vid:04x}" if port.vid is not None else "?????"
            pid_str = f"{port.pid:04x}" if port.pid is not None else "?????"
            port_list.append(f"{port.device} - {port.description} (VID:{vid_str}, PID:{pid_str})")
            self.log(f"ポート検出: {port.device} - {port.description} (VID:{vid_str}, PID:{pid_str})")
        
        if port_list:
            self.port_combobox['values'] = port_list
            self.port_combobox.current(0)
            self.log(f"利用可能なポート: {len(port_list)}個見つかりました")
        else:
            self.port_combobox['values'] = ["利用可能なポートがありません"]
            self.port_combobox.current(0)
            self.log("利用可能なシリアルポートが見つかりません")

    def toggle_connection(self):
        """接続/切断を切り替え"""
        if not self.is_connected:
            self.connect()
        else:
            self.disconnect()

    def connect(self):
        """シリアルポートに接続"""
        # 選択されたポート文字列から実際のデバイス名を抽出
        # 例: "/dev/ttyUSB0 - USB Serial (VID:1234, PID:5678)" から "/dev/ttyUSB0" を取得
        port_str = self.port_combobox.get()
        # 最初のスペースまでを取得（デバイス名にはスペースが含まれないと仮定）
        port_device = port_str.split(" ")[0]
        baud_rate = int(self.baud_combobox.get())
        
        try:
            self.serial_connection = serial.Serial(port_device, baud_rate, timeout=1)
            self.is_connected = True
            self.status_var.set(f"接続済み: {port_device} ({baud_rate} bps)")
            self.connect_button.config(text="切断")
            self.send_button.config(state=tk.NORMAL)
            self.log(f"{port_device} に接続しました (ボーレート: {baud_rate})")
        except serial.SerialException as e:
            messagebox.showerror("接続エラー", f"ポートに接続できません: {e}")
            self.log(f"接続エラー: {e}")

    def disconnect(self):
        """シリアルポートから切断"""
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
        
        self.is_connected = False
        self.status_var.set("未接続")
        self.connect_button.config(text="接続")
        self.send_button.config(state=tk.DISABLED)
        self.log("切断しました")

    def send_animation(self):
        """アニメーションデータを送信"""
        if not self.is_connected or not self.serial_connection:
            messagebox.showerror("エラー", "シリアルポートに接続されていません")
            return
        
        animation_type = self.animation_type.get()
        
        # 別スレッドでアニメーションを送信（UIをブロックしないため）
        threading.Thread(target=self._send_animation_thread, args=(animation_type,), daemon=True).start()

    def _send_animation_thread(self, animation_type):
        """アニメーション送信のスレッド処理"""
        self.send_button.config(state=tk.DISABLED)
        self.log(f"{animation_type} アニメーションを作成中...")
        
        try:
            # アニメーションタイプに応じてデータを作成
            if animation_type == "sample":
                data = self.create_sample_animation()
            elif animation_type == "random":
                data = self.create_random_animation()
            elif animation_type == "wave":
                data = self.create_wave_animation()
            else:
                self.log(f"不明なアニメーションタイプ: {animation_type}")
                self.send_button.config(state=tk.NORMAL)
                return
            
            # データを送信
            self.log("データを送信中...")
            self.serial_connection.write(data.encode('utf-8'))
            self.log(f"データを送信しました ({len(data)} バイト)")
            
        except serial.SerialException as e:
            self.log(f"送信エラー: {e}")
            messagebox.showerror("送信エラー", f"データの送信に失敗しました: {e}")
        except Exception as e:
            self.log(f"エラー: {e}")
            messagebox.showerror("エラー", f"予期しないエラーが発生しました: {e}")
        finally:
            self.send_button.config(state=tk.NORMAL)

    def create_sample_animation(self) -> str:
        """サンプルアニメーションデータを作成"""
        # カラーパレットの定義
        palette = ['#FF0000', '#00FF00', '#0000FF', '#FFFF00', '#FF00FF', '#00FFFF', '#FFFFFF', '#000000']
        
        # データ構築
        data = f"palette: {', '.join(palette)}\n"
        
        # 簡単なアニメーションフレームを作成（点滅パターン）
        for i in range(10):
            # 偶数フレームは赤と青のチェック柄、奇数フレームは緑と黄色のチェック柄
            color_indices = [0] * (LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT)
            
            for y in range(LED_TOTAL_HEIGHT):
                for x in range(LED_TOTAL_WIDTH):
                    # チェック柄パターン
                    is_even_cell = (x % 2 == 0 and y % 2 == 0) or (x % 2 == 1 and y % 2 == 1)
                    
                    if i % 2 == 0:
                        color_indices[y * LED_TOTAL_WIDTH + x] = 0 if is_even_cell else 2  # 赤と青
                    else:
                        color_indices[y * LED_TOTAL_WIDTH + x] = 1 if is_even_cell else 3  # 緑と黄色
            
            # フレームデータを追加（時間は100ms * フレーム番号）
            data += f"flame: {100 * (i + 1)}, {', '.join(map(str, color_indices))}\n"
        
        # 終了マーカー
        data += "END\n"
        
        return data

    def create_random_animation(self) -> str:
        """ランダムなアニメーションデータを作成"""
        # カラーパレットの定義
        palette = [
            '#FF0000', '#00FF00', '#0000FF', '#FFFF00', 
            '#FF00FF', '#00FFFF', '#FFFFFF', '#000000',
            '#FFA500', '#800080', '#008000', '#800000',
            '#008080', '#000080', '#FFC0CB', '#A52A2A'
        ]
        
        # データ構築
        data = f"palette: {', '.join(palette)}\n"
        
        # ランダムなアニメーションフレームを作成
        for i in range(20):
            color_indices = []
            
            for _ in range(LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT):
                color_indices.append(random.randint(0, len(palette) - 1))
            
            # フレームデータを追加（時間は100ms * フレーム番号）
            data += f"flame: {100 * (i + 1)}, {', '.join(map(str, color_indices))}\n"
        
        # 終了マーカー
        data += "END\n"
        
        return data

    def create_wave_animation(self) -> str:
        """波紋アニメーションデータを作成"""
        # カラーパレットの定義
        palette = [
            '#000000',  # 黒（背景）
            '#0000FF',  # 青（薄い）
            '#0000CC',  # 青（中間）
            '#000099',  # 青（濃い）
            '#000066',  # 青（さらに濃い）
            '#000033',  # 青（最も濃い）
        ]
        
        # データ構築
        data = f"palette: {', '.join(palette)}\n"
        
        # 波紋アニメーションフレームを作成
        frames = 30
        for frame in range(frames):
            color_indices = [0] * (LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT)
            
            # 中心点
            center_x = LED_TOTAL_WIDTH // 2
            center_y = LED_TOTAL_HEIGHT // 2
            
            # 波の半径（フレームごとに大きくなる）
            radius = frame * 2
            thickness = 5
            
            for y in range(LED_TOTAL_HEIGHT):
                for x in range(LED_TOTAL_WIDTH):
                    # 中心からの距離を計算
                    distance = ((x - center_x) ** 2 + (y - center_y) ** 2) ** 0.5
                    
                    # 波の輪の中にあるピクセルに色を設定
                    if radius - thickness < distance < radius:
                        # 距離に応じて色の濃さを変える
                        color_index = min(5, max(1, int((distance % thickness) / thickness * 5) + 1))
                        color_indices[y * LED_TOTAL_WIDTH + x] = color_index
            
            # フレームデータを追加（時間は50ms * フレーム番号）
            data += f"flame: {50 * (frame + 1)}, {', '.join(map(str, color_indices))}\n"
        
        # 終了マーカー
        data += "END\n"
        
        return data

def main():
    root = tk.Tk()
    app = LEDMatrixController(root)
    root.mainloop()

if __name__ == "__main__":
    main()
