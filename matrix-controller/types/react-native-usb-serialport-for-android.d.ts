declare module 'react-native-usb-serialport-for-android' {
  export enum Parity {
    NONE = 0,
    ODD = 1,
    EVEN = 2,
    MARK = 3,
    SPACE = 4
  }

  export enum Codes {
    SUCCESS = 0,
    FAIL = 1,
    PERMISSION_DENIED = 2,
    DEVICE_NOT_FOUND = 3,
    DEVICE_BUSY = 4,
    INVALID_PARAMETER = 5,
    IO_ERROR = 6,
    TIMEOUT = 7
  }

  export interface SerialPortConfig {
    baudRate: number;
    dataBits: number;
    stopBits: number;
    parity: Parity;
    dtr?: boolean;
    rts?: boolean;
    sleepOnPause?: boolean;
  }

  export interface SerialDevice {
    deviceId: number;
    deviceName: string;
    vendorId: number;
    productId: number;
  }

  export interface UsbSerialManager {
    /**
     * リストアップ可能なUSBデバイスを取得します
     */
    list(): Promise<SerialDevice[]>;

    /**
     * USBデバイスに接続します
     * @param deviceId デバイスID
     * @param config シリアルポート設定
     */
    connect(deviceId: number, config: SerialPortConfig): Promise<void>;

    /**
     * USBデバイスから切断します
     * @param deviceId デバイスID
     */
    disconnect(deviceId: number): Promise<void>;

    /**
     * USBデバイスにデータを書き込みます
     * @param deviceId デバイスID
     * @param data 書き込むデータ
     */
    write(deviceId: number, data: string): Promise<void>;

    /**
     * USBデバイスからデータを読み込みます
     * @param deviceId デバイスID
     * @param timeout タイムアウト（ミリ秒）
     */
    read(deviceId: number, timeout?: number): Promise<string>;

    /**
     * USBデバイスの接続状態を確認します
     * @param deviceId デバイスID
     */
    isConnected(deviceId: number): Promise<boolean>;

    /**
     * USBデバイスの権限を要求します
     * @param deviceId デバイスID
     */
    requestPermission(deviceId: number): Promise<boolean>;
  }

  export const UsbSerialManager: UsbSerialManager;
}
