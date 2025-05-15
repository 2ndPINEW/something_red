#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import serial.tools.list_ports
import time
import argparse
import sys
import os
from typing import List, Dict, Optional, Tuple
import json
import random

# LEDマトリックスのサイズ定数
LED_TOTAL_WIDTH = 108
LED_TOTAL_HEIGHT = 36

def list_serial_ports():
    """利用可能なシリアルポートを一覧表示"""
    ports = serial.tools.list_ports.comports()
    
    if not ports:
        print("利用可能なシリアルポートが見つかりません")
        return []
    
    print("利用可能なシリアルポート:")
    for i, port in enumerate(ports):
        # VIDとPIDがNoneの場合のハンドリング
        vid_str = f"{port.vid:04x}" if port.vid is not None else "?????"
        pid_str = f"{port.pid:04x}" if port.pid is not None else "?????"
        print(f"[{i}] {port.device} - {port.description} (VID:{vid_str}, PID:{pid_str})")
    
    return ports

def connect_to_port(port_name: str, baud_rate: int = 115200) -> Optional[serial.Serial]:
    """指定したポートに接続"""
    try:
        ser = serial.Serial(port_name, baud_rate, timeout=1)
        print(f"{port_name} に接続しました (ボーレート: {baud_rate})")
        return ser
    except serial.SerialException as e:
        print(f"エラー: {port_name} に接続できません - {e}")
        return None

def create_sample_animation() -> str:
    """サンプルアニメーションデータを作成"""
    # カラーパレットの定義
    palette = ['#FF0000', '#00FF00', '#0000FF', '#FFFF00', '#FF00FF', '#00FFFF', '#FFFFFF', '#000000']
    
    # データ構築
    data = f"palette: {', '.join(palette)}\n"
    
    # 簡単なアニメーションフレームを作成（点滅パターン）
    for i in range(3):
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

def create_random_animation() -> str:
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
    for i in range(3):
        color_indices = []
        
        for _ in range(LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT):
            color_indices.append(random.randint(0, len(palette) - 1))
        
        # フレームデータを追加（時間は100ms * フレーム番号）
        data += f"flame: {100 * (i + 1)}, {', '.join(map(str, color_indices))}\n"
    
    # 終了マーカー
    data += "END\n"
    
    return data

def create_wave_animation() -> str:
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
    frames = 10
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

def send_animation_data(ser: serial.Serial, data: str) -> bool:
    """アニメーションデータを送信"""
    try:
        # データを送信
        ser.write(data.encode('utf-8'))
        print(f"データを送信しました ({len(data)} バイト)")
        return True
    except serial.SerialException as e:
        print(f"送信エラー: {e}")
        return False

def main():
    """メイン関数"""
    parser = argparse.ArgumentParser(description='LEDマトリックスコントローラー')
    parser.add_argument('--port', type=str, help='シリアルポート名')
    parser.add_argument('--baud', type=int, default=115200, help='ボーレート (デフォルト: 115200)')
    parser.add_argument('--animation', type=str, default='sample', 
                        choices=['sample', 'random', 'wave'], 
                        help='アニメーションタイプ (デフォルト: sample)')
    args = parser.parse_args()

    # ポートが指定されていない場合は一覧を表示
    if not args.port:
        ports = list_serial_ports()
        if not ports:
            return
        
        try:
            port_index = int(input("\nポート番号を選択してください: "))
            if port_index < 0 or port_index >= len(ports):
                print("無効なポート番号です")
                return
            port_name = ports[port_index].device
        except ValueError:
            print("無効な入力です")
            return
    else:
        port_name = args.port

    # 選択したポートに接続
    ser = connect_to_port(port_name, args.baud)
    if not ser:
        return

    try:
        # アニメーションタイプに応じてデータを作成
        if args.animation == 'sample':
            print("サンプルアニメーションを作成中...")
            data = create_sample_animation()
        elif args.animation == 'random':
            print("ランダムアニメーションを作成中...")
            data = create_random_animation()
        elif args.animation == 'wave':
            print("波紋アニメーションを作成中...")
            data = create_wave_animation()
        
        # データを送信
        print("データを送信中...")
        print(data)
        success = send_animation_data(ser, data)
        
        if success:
            print("送信完了！")
        else:
            print("送信失敗")
    
    finally:
        # 接続を閉じる
        if ser and ser.is_open:
            ser.close()
            print(f"{port_name} との接続を閉じました")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n終了します")
        sys.exit(0)
