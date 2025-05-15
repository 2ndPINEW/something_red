import { Image } from 'expo-image';
import { Platform, StyleSheet, Button, ScrollView, TouchableOpacity, Alert } from 'react-native';
import { useState, useEffect } from 'react';

import { HelloWave } from '@/components/HelloWave';
import ParallaxScrollView from '@/components/ParallaxScrollView';
import { ThemedText } from '@/components/ThemedText';
import { ThemedView } from '@/components/ThemedView';

import { UsbSerialManager, Parity, Codes } from "react-native-usb-serialport-for-android";

// シリアルデバイスの型定義
type SerialDevice = {
  deviceId: number;
  deviceName: string;
  vendorId: number;
  productId: number;
};

export default function HomeScreen() {
  // 状態管理
  const [devices, setDevices] = useState<SerialDevice[]>([]);
  const [selectedDevice, setSelectedDevice] = useState<SerialDevice | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<string>('未接続');
  const [isConnected, setIsConnected] = useState<boolean>(false);

  // デバイス一覧を取得する関数
  const fetchDevices = async () => {
    try {
      const deviceList = await UsbSerialManager.list();
      console.log('デバイス一覧:', deviceList);
      setDevices(deviceList || []);
      
      if (deviceList && deviceList.length === 0) {
        Alert.alert('デバイスが見つかりません', 'USBデバイスを接続してください');
      }
    } catch (error) {
      console.error('デバイス一覧取得エラー:', error);
      Alert.alert('エラー', 'デバイス一覧の取得に失敗しました');
    }
  };

  // デバイスに接続する関数
  const connectToDevice = async (device: SerialDevice) => {
    try {
      setConnectionStatus('接続中...');
      
      // 接続設定
      await UsbSerialManager.connect(
        device.deviceId,
        {
          baudRate: 115200,
          dataBits: 8,
          stopBits: 1,
          parity: Parity.NONE,
        }
      );
      
      setSelectedDevice(device);
      setIsConnected(true);
      setConnectionStatus(`接続済み: ${device.deviceName}`);
      
      Alert.alert('接続成功', `${device.deviceName}に接続しました`);
    } catch (error) {
      console.error('接続エラー:', error);
      setConnectionStatus('接続失敗');
      setIsConnected(false);
      Alert.alert('接続エラー', '接続に失敗しました');
    }
  };

  // デバイスから切断する関数
  const disconnectFromDevice = async () => {
    if (!selectedDevice) return;
    
    try {
      await UsbSerialManager.disconnect(selectedDevice.deviceId);
      setSelectedDevice(null);
      setIsConnected(false);
      setConnectionStatus('未接続');
      Alert.alert('切断完了', 'デバイスから切断しました');
    } catch (error) {
      console.error('切断エラー:', error);
      Alert.alert('切断エラー', '切断に失敗しました');
    }
  };

  // サンプルデータを送信する関数
  const sendSampleData = async () => {
    if (!selectedDevice || !isConnected) {
      Alert.alert('エラー', 'デバイスに接続されていません');
      return;
    }

    try {
      // サンプルデータの作成
      const sampleData = createSampleAnimation();
      
      console.log('送信データ:', sampleData);
      
      // データを送信
      await UsbSerialManager.write(selectedDevice.deviceId, sampleData);
      
      Alert.alert('送信完了', 'アニメーションデータを送信しました');
    } catch (error) {
      console.error('送信エラー:', error);
      Alert.alert('送信エラー', 'データの送信に失敗しました');
    }
  };

  // サンプルアニメーションデータを作成する関数
  const createSampleAnimation = (): string => {
    // カラーパレットの定義（最大256色）
    const palette = [
      '#FF0000', // 赤 (0)
      '#00FF00', // 緑 (1)
      '#0000FF', // 青 (2)
      '#FFFF00', // 黄 (3)
      '#FF00FF', // マゼンタ (4)
      '#00FFFF', // シアン (5)
      '#FFFFFF', // 白 (6)
      '#000000'  // 黒 (7)
    ];
    
    // データ構築
    let data = `palette: ${palette.join(', ')}\n`;
    
    // LED行列のサイズ定数
    const LED_TOTAL_WIDTH = 108;
    const LED_TOTAL_HEIGHT = 36;
    const TOTAL_PIXELS = LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT;
    
    // 簡単なアニメーションフレームを作成（点滅パターン）
    for (let i = 0; i < 10; i++) {
      // 各フレームのピクセルデータを作成
      const pixelIndices = new Array(TOTAL_PIXELS);
      
      for (let y = 0; y < LED_TOTAL_HEIGHT; y++) {
        for (let x = 0; x < LED_TOTAL_WIDTH; x++) {
          const pixelIndex = y * LED_TOTAL_WIDTH + x;
          
          // チェック柄パターン
          const isEvenCell = (x % 2 === 0 && y % 2 === 0) || (x % 2 === 1 && y % 2 === 1);
          
          if (i % 2 === 0) {
            // 偶数フレーム: 赤と青のチェック柄
            pixelIndices[pixelIndex] = isEvenCell ? 0 : 2; // 赤(0)と青(2)
          } else {
            // 奇数フレーム: 緑と黄色のチェック柄
            pixelIndices[pixelIndex] = isEvenCell ? 1 : 3; // 緑(1)と黄(3)
          }
        }
      }
      
      // フレームデータを追加（時間は200ms * フレーム番号）
      data += `flame: ${200 * (i + 1)}, ${pixelIndices.join(', ')}\n`;
    }
    
    // 終了マーカー
    data += 'END\n';
    
    return data;
  };

  // 虹色のアニメーションを作成する関数
  const createRainbowAnimation = (): string => {
    // 虹色のパレット（7色+黒）
    const palette = [
      '#FF0000', // 赤
      '#FF7F00', // オレンジ
      '#FFFF00', // 黄
      '#00FF00', // 緑
      '#0000FF', // 青
      '#4B0082', // インディゴ
      '#9400D3', // バイオレット
      '#000000'  // 黒
    ];
    
    let data = `palette: ${palette.join(', ')}\n`;
    
    // LED行列のサイズ定数
    const LED_TOTAL_WIDTH = 108;
    const LED_TOTAL_HEIGHT = 36;
    const TOTAL_PIXELS = LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT;
    
    // 虹のアニメーションフレームを作成
    for (let i = 0; i < 7; i++) {
      const pixelIndices = new Array(TOTAL_PIXELS);
      
      for (let y = 0; y < LED_TOTAL_HEIGHT; y++) {
        for (let x = 0; x < LED_TOTAL_WIDTH; x++) {
          const pixelIndex = y * LED_TOTAL_WIDTH + x;
          
          // 縦方向に虹色の帯を作成し、フレームごとに移動させる
          const colorIndex = (x + i) % 7; // 7色を循環
          pixelIndices[pixelIndex] = colorIndex;
        }
      }
      
      // フレームデータを追加（時間は300ms固定）
      data += `flame: 300, ${pixelIndices.join(', ')}\n`;
    }
    
    data += 'END\n';
    return data;
  };

  // 波紋アニメーションを作成する関数
  const createRippleAnimation = (): string => {
    // 青系のグラデーションパレット
    const palette = [
      '#000033', // 暗い青
      '#000066',
      '#000099',
      '#0000CC',
      '#0000FF', // 鮮やかな青
      '#3333FF',
      '#6666FF',
      '#9999FF',
      '#CCCCFF',
      '#FFFFFF'  // 白
    ];
    
    let data = `palette: ${palette.join(', ')}\n`;
    
    // LED行列のサイズ定数
    const LED_TOTAL_WIDTH = 108;
    const LED_TOTAL_HEIGHT = 36;
    const TOTAL_PIXELS = LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT;
    
    // 中心点
    const centerX = Math.floor(LED_TOTAL_WIDTH / 2);
    const centerY = Math.floor(LED_TOTAL_HEIGHT / 2);
    
    // 波紋アニメーションフレームを作成
    for (let i = 0; i < 20; i++) {
      const pixelIndices = new Array(TOTAL_PIXELS);
      const radius = i * 3; // 波紋の半径
      
      for (let y = 0; y < LED_TOTAL_HEIGHT; y++) {
        for (let x = 0; x < LED_TOTAL_WIDTH; x++) {
          const pixelIndex = y * LED_TOTAL_WIDTH + x;
          
          // 中心からの距離を計算
          const distance = Math.sqrt(Math.pow(x - centerX, 2) + Math.pow(y - centerY, 2));
          
          // 波紋の輪の幅
          const ringWidth = 3;
          
          // 距離に基づいて色を決定
          if (Math.abs(distance - radius) < ringWidth) {
            // 波紋の輪の部分
            pixelIndices[pixelIndex] = 9; // 白
          } else if (distance < radius) {
            // 波紋の内側
            const colorIndex = Math.min(8, Math.floor(distance / 5)) % palette.length;
            pixelIndices[pixelIndex] = colorIndex;
          } else {
            // 波紋の外側
            pixelIndices[pixelIndex] = 0; // 暗い青
          }
        }
      }
      
      // フレームデータを追加（時間は100ms固定）
      data += `flame: 100, ${pixelIndices.join(', ')}\n`;
    }
    
    data += 'END\n';
    return data;
  };

  // サンプルアニメーションを選択して送信する関数
  const sendAnimation = async (animationType: string) => {
    if (!selectedDevice || !isConnected) {
      Alert.alert('エラー', 'デバイスに接続されていません');
      return;
    }

    try {
      let animationData = '';
      
      switch (animationType) {
        case 'checker':
          animationData = createSampleAnimation();
          break;
        case 'rainbow':
          animationData = createRainbowAnimation();
          break;
        case 'ripple':
          animationData = createRippleAnimation();
          break;
        default:
          animationData = createSampleAnimation();
      }
      
      // データを送信
      await UsbSerialManager.write(selectedDevice.deviceId, animationData);
      
      Alert.alert('送信完了', `${animationType}アニメーションを送信しました`);
    } catch (error) {
      console.error('送信エラー:', error);
      Alert.alert('送信エラー', 'データの送信に失敗しました');
    }
  };

  return (
    <ParallaxScrollView
      headerBackgroundColor={{ light: '#A1CEDC', dark: '#1D3D47' }}
      headerImage={
        <Image
          source={require('@/assets/images/partial-react-logo.png')}
          style={styles.reactLogo}
        />
      }>
      <ThemedView style={styles.titleContainer}>
        <ThemedText type="title">マトリックスコントローラー</ThemedText>
        <HelloWave />
      </ThemedView>
      
      <ThemedView style={styles.stepContainer}>
        <ThemedText type="subtitle">接続状態: {connectionStatus}</ThemedText>
        
        <TouchableOpacity 
          style={styles.button} 
          onPress={fetchDevices}
        >
          <ThemedText style={styles.buttonText}>USBデバイス一覧を取得</ThemedText>
        </TouchableOpacity>
        
        {isConnected ? (
          <>
            <TouchableOpacity 
              style={styles.button} 
              onPress={disconnectFromDevice}
            >
              <ThemedText style={styles.buttonText}>切断する</ThemedText>
            </TouchableOpacity>
            
            <ThemedText type="subtitle" style={{marginTop: 16}}>アニメーション送信</ThemedText>
            
            <TouchableOpacity 
              style={[styles.button, styles.sendButton]} 
              onPress={() => sendAnimation('checker')}
            >
              <ThemedText style={styles.buttonText}>チェック柄アニメーション</ThemedText>
            </TouchableOpacity>
            
            <TouchableOpacity 
              style={[styles.button, styles.rainbowButton]} 
              onPress={() => sendAnimation('rainbow')}
            >
              <ThemedText style={styles.buttonText}>虹色アニメーション</ThemedText>
            </TouchableOpacity>
            
            <TouchableOpacity 
              style={[styles.button, styles.rippleButton]} 
              onPress={() => sendAnimation('ripple')}
            >
              <ThemedText style={styles.buttonText}>波紋アニメーション</ThemedText>
            </TouchableOpacity>
          </>
        ) : null}
      </ThemedView>
      
      {devices.length > 0 && !isConnected ? (
        <ThemedView style={styles.stepContainer}>
          <ThemedText type="subtitle">利用可能なデバイス</ThemedText>
          <ScrollView style={styles.deviceList}>
            {devices.map((device, index) => (
              <TouchableOpacity
                key={`${device.deviceId}-${index}`}
                style={styles.deviceItem}
                onPress={() => connectToDevice(device)}
              >
                <ThemedText>{device.deviceName || `デバイス ${device.deviceId}`}</ThemedText>
                <ThemedText style={styles.deviceDetails}>
                  VID: {device.vendorId.toString(16)}, PID: {device.productId.toString(16)}
                </ThemedText>
              </TouchableOpacity>
            ))}
          </ScrollView>
        </ThemedView>
      ) : null}
      
      <ThemedView style={styles.stepContainer}>
        <ThemedText type="subtitle">使い方</ThemedText>
        <ThemedText>
          1. 「USBデバイス一覧を取得」ボタンを押してデバイスを検索
        </ThemedText>
        <ThemedText>
          2. 表示されたデバイスをタップして接続
        </ThemedText>
        <ThemedText>
          3. アニメーションボタンを押してLEDマトリックスにデータを送信
        </ThemedText>
      </ThemedView>
      
      <ThemedView style={styles.stepContainer}>
        <ThemedText type="subtitle">データ形式</ThemedText>
        <ThemedText style={styles.codeBlock}>
          palette: #ffffff, #000000{'\n'}
          flame: 時間, 0の色index, 1の色index ...{'\n'}
          flame: 時間, 0の色index, 1の色index ...{'\n'}
          END
        </ThemedText>
        <ThemedText style={{marginTop: 8}}>
          ※ カラーパレットの最大数は256色です
        </ThemedText>
      </ThemedView>
    </ParallaxScrollView>
  );
}

const styles = StyleSheet.create({
  titleContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  stepContainer: {
    gap: 8,
    marginBottom: 16,
  },
  reactLogo: {
    height: 178,
    width: 290,
    bottom: 0,
    left: 0,
    position: 'absolute',
  },
  button: {
    backgroundColor: '#2196F3',
    padding: 12,
    borderRadius: 8,
    marginVertical: 8,
    alignItems: 'center',
  },
  sendButton: {
    backgroundColor: '#4CAF50',
  },
  rainbowButton: {
    backgroundColor: '#9C27B0',
  },
  rippleButton: {
    backgroundColor: '#00BCD4',
  },
  buttonText: {
    color: 'white',
    fontWeight: 'bold',
  },
  deviceList: {
    maxHeight: 200,
  },
  deviceItem: {
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#CCCCCC',
  },
  deviceDetails: {
    fontSize: 12,
    opacity: 0.7,
  },
  codeBlock: {
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    backgroundColor: 'rgba(0, 0, 0, 0.05)',
    padding: 8,
    borderRadius: 4,
  },
});
