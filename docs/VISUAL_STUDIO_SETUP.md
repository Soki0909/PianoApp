# Visual Studio セットアップガイド

**3D電子ピアノアプリケーション開発環境構築**

このガイドでは、Visual Studioを使用して3D電子ピアノアプリケーションを開発・実行するための詳細な手順を説明します。

## ✅ 動作確認済み環境

以下の環境で正常にビルド・実行が確認されています（2025年8月26日現在）：

### システム環境
- **OS**: Windows 11 Home (ビルド 26100, バージョン 24H2)
- **システム**: Panasonic Connect Let'snote/TOUGHBOOK CFFV3-1
- **CPU**: Intel64 Family 6 Model 154 (~2100 MHz, 12コア/16スレッド)
- **メモリ**: 16.05 GB (16,054 MB)
- **GPU**: Intel(R) Iris(R) Xe Graphics (ドライバー: 32.0.101.6314)
- **ネットワーク**: Intel Wi-Fi 6E AX211 160MHz, Intel Ethernet Connection (16) I219-V

### 開発環境
- **Visual Studio**: 2022 Community Edition
  - **IDE バージョン**: 17.14.36408.4
  - **開発者コマンドプロンプト**: Version 17.14.12
  - **インストールパス**: `C:\Program Files\Microsoft Visual Studio\2022\Community\`
- **MSVC ツールセット**: 14.44.35207 (v143)
- **MSVC コンパイラ**: Version 19.44.35214 for x86
- **MSBuild**: Version 17.14.18+a338add32 (.NET Framework)
- **Windows SDK**: 10.0.26100 (`C:\Program Files (x86)\Windows Kits\10\`)
- **プラットフォーム**: x64
- **ビルド構成**: Debug/Release 両対応

### オーディオ環境
- Realtek High Definition Audio
- インテル® スマート・サウンド・テクノロジー (複数デバイス)
- USB オーディオ対応

### ビルド成果物
- **実行ファイル**: `x64\Debug\PianoApp.exe` (1,333,760 bytes)
- **最終ビルド日時**: 2025年7月19日 13:09:02
- **動作状態**: 正常動作確認済み

## 🔧 必要なソフトウェア

### Visual Studio Community/Professional/Enterprise
- **推奨バージョン**: Visual Studio 2022 (17.0以降)
- **最小バージョン**: Visual Studio 2019 (16.0以降)
- **動作確認済み**: Visual Studio 2022 Community Edition 17.14.36408.4
  - MSVC v143 (14.44.35207)
  - 開発者コマンドプロンプト 17.14.12
- **必要ワークロード**: 
  - C++によるデスクトップ開発
  - Windows 10/11 SDK

### Windows SDK
- **推奨バージョン**: Windows 11 SDK (10.0.22000.0)
- **最小バージョン**: Windows 10 SDK (10.0.19041.0)
- **動作確認済み**: Windows SDK 10.0.26100 (Windows 11 Home ビルド 26100)

### ハードウェア要件
- **CPU**: マルチコア推奨 (動作確認: Intel64 Family 6 Model 154 ~2100MHz)
- **メモリ**: 8GB以上推奨 (動作確認: 16GB)
- **GPU**: OpenGL対応 (動作確認: Intel Iris Xe Graphics)
- **オーディオ**: Windows Audio対応 (動作確認: Realtek HD Audio)
- **ストレージ**: 100MB以上の空き容量
- **システム**: x64-based PC (動作確認: Panasonic Let'snote CFFV3-1)

## 📋 インストール手順

### Step 1: Visual Studio インストール

1. **Visual Studio Installer**を起動
2. **ワークロード**タブで以下を選択:
   - ✅ **C++によるデスクトップ開発**
   
3. **個別のコンポーネント**タブで以下を確認:
   - ✅ MSVC v143 - VS 2022 C++ x64/x86 ビルドツール
   - ✅ Windows 11 SDK (10.0.22000.0)
   - ✅ CMake tools for Visual Studio
   - ✅ Git for Windows

4. **インストール**をクリック

### Step 2: プロジェクトセットアップ

1. **リポジトリクローン**:
   ```bash
   git clone [リポジトリURL]
   cd PianoApp
   ```

2. **Visual Studioでソリューション開く**:
   - `PianoApp.sln`をダブルクリック
   - または Visual Studio → ファイル → 開く → プロジェクト/ソリューション

3. **NuGetパッケージ復元**:
   - Visual Studioが自動的に復元を開始
   - 手動復元: ソリューションエクスプローラー → ソリューション右クリック → **NuGetパッケージの復元**

## ⚙️ プロジェクト設定

### ビルド構成の確認

1. **プラットフォーム設定**:
   - ツールバーのプラットフォーム: **x64**を選択
   - ❌ **x86** (32bit) は非推奨

2. **構成設定**:
   - 開発時: **Debug**
   - 配布用: **Release**

### プロジェクトプロパティ確認

**ソリューションエクスプローラー** → **PianoApp**プロジェクト右クリック → **プロパティ**

#### 構成プロパティ → 全般
```
プラットフォーム ツールセット: v143 (Visual Studio 2022)
  ※動作確認済み: MSVC v143 (14.44.35207)
Windows SDK バージョン: 10.0 (最新がインストールされている)
  ※動作確認済み: 10.0.26100
文字セット: Unicode文字セットを使用する
```

#### 構成プロパティ → C/C++ → 全般
```
追加のインクルード ディレクトリ: (NuGetが自動設定)
警告レベル: レベル3 (/W3)
SDL チェック: はい (/sdl)
```

#### 構成プロパティ → リンカー → 全般
```
追加のライブラリ ディレクトリ: (NuGetが自動設定)
```

#### 構成プロパティ → リンカー → 入力
```
追加の依存ファイル: (NuGetが自動設定)
- freeglut.lib
- glew32.lib
- opengl32.lib
```

## 🔍 NuGetパッケージ詳細

### インストール済みパッケージ

`packages.config`で管理:
```xml
<package id="nupengl.core" version="0.1.0.1" targetFramework="native" />
<package id="nupengl.core.redist" version="0.1.0.1" targetFramework="native" />
```

### 提供されるライブラリ

**nupengl.core**:
- `GL/glut.h` - GLUT ヘッダー
- `GL/glew.h` - GLEW ヘッダー
- `freeglut.lib` - GLUT スタティックライブラリ
- `glew32.lib` - GLEW スタティックライブラリ

**nupengl.core.redist**:
- `freeglut.dll` - GLUT ランタイム
- `glew32.dll` - GLEW ランタイム
- `glfw3.dll` - GLFW ランタイム（未使用）

### 手動パッケージ復元

問題が発生した場合:

1. **パッケージマネージャーコンソール**を開く:
   - メニュー: ツール → NuGet パッケージマネージャー → パッケージマネージャーコンソール

2. **復元コマンド実行**:
   ```powershell
   Update-Package -reinstall
   ```

3. **キャッシュクリア** (必要時):
   ```powershell
   nuget locals all -clear
   ```

## 🚀 ビルドと実行

### ビルド手順

1. **ソリューションのクリーン**:
   - メニュー: ビルド → ソリューションのクリーン

2. **ソリューションのリビルド**:
   - メニュー: ビルド → ソリューションのリビルド
   - ショートカット: `Ctrl + Shift + B`

3. **エラーチェック**:
   - **エラー一覧**ウィンドウでコンパイルエラーを確認
   - 出力ウィンドウで詳細ログを確認

### 実行手順

1. **デバッグ実行**:
   - メニュー: デバッグ → デバッグの開始
   - ショートカット: `F5`
   - ブレークポイントでの停止可能

2. **デバッグなし実行**:
   - メニュー: デバッグ → デバッグなしで開始
   - ショートカット: `Ctrl + F5`
   - 高速実行

### 作業ディレクトリ設定

重要: データファイル（3Dモデル、音色、楽譜）の正常読み込みのため

**プロジェクトプロパティ** → **構成プロパティ** → **デバッグ**:
```
作業ディレクトリ: $(ProjectDir)
```

これにより、`PianoApp.c`と同じフォルダが作業ディレクトリになります。

## 🐛 デバッグ機能

### ブレークポイント設定

1. **コード行クリック**: 行番号の左をクリック
2. **ショートカット**: `F9`
3. **条件付きブレークポイント**: ブレークポイント右クリック → 条件

### 変数ウォッチ

**デバッグ実行中**:
- **自動**ウィンドウ: 現在のコンテキストの変数
- **ローカル**ウィンドウ: ローカル変数
- **ウォッチ**ウィンドウ: 手動追加した変数

### よく監視する変数

```c
// ピアノ状態
g_piano_keys[0].envelope_state     // 最初の鍵盤の状態
g_current_timbre_index             // 選択中の音色
g_current_octave_shift             // オクターブシフト

// カメラ状態
g_camera_pos[0], g_camera_pos[1], g_camera_pos[2]    // カメラ位置
g_camera_yaw, g_camera_pitch                         // カメラ角度

// シーケンサー状態
g_is_sequencer_playing             // 自動演奏フラグ
g_sequence_index                   // 現在の再生位置
```

## ⚠️ よくある問題と解決法

> **💡 動作実績**: 上記「動作確認済み環境」において、以下の問題は発生せず正常動作しています。  
> **確認システム**: Windows 11 Home (24H2) + Panasonic Let'snote CFFV3-1

### ❌ ビルドエラー: "freeglut.lib が見つからない"

**原因**: NuGetパッケージが正しく復元されていない

**解決法**:
1. ソリューションエクスプローラー → ソリューション右クリック → **NuGetパッケージの復元**
2. プロジェクトを一度閉じて再度開く
3. Visual Studioを管理者権限で実行

### ❌ 実行エラー: "freeglut.dll が見つからない"

**原因**: ランタイムDLLが実行ファイルと同じフォルダにない

**解決法**:
1. **出力ディレクトリ確認**:
   - Debug: `x64/Debug/`
   - Release: `x64/Release/`

2. **DLL自動コピー確認**:
   - NuGetが自動的にDLLをコピーするはず
   - 手動コピー: `packages/nupengl.core.redist.0.1.0.1/build/native/bin/x64/` から実行フォルダへ

### ❌ 実行エラー: "object/Body.obj が見つからない"

**原因**: 作業ディレクトリが正しく設定されていない

**解決法**:
1. **作業ディレクトリ確認**:
   - プロジェクトプロパティ → デバッグ → 作業ディレクトリ: `$(ProjectDir)`

2. **ファイル存在確認**:
   ```
   PianoApp/
   ├── PianoApp.c
   ├── object/
   │   ├── Body.obj
   │   └── ...
   ```

### ❌ 実行エラー: "音声デバイスの初期化に失敗"

**原因**: Windows Audio サービスの問題またはデバイス競合

**解決法**:
1. **他のオーディオアプリ終了**: 音楽プレイヤー、通話アプリなど
2. **既定のデバイス確認**: Windowsサウンド設定 → 出力デバイス
3. **Windows Audio サービス再起動**:
   ```cmd
   net stop audiosrv
   net start audiosrv
   ```

### ❌ 実行時: 画面が真っ黒

**原因**: OpenGLドライバーまたはGPU問題

**解決法**:
1. **GPU ドライバー更新**: NVIDIA/AMD/Intel公式サイト
2. **統合GPU確認**: ノートPCの場合、統合GPUが使われている可能性
3. **OpenGL バージョン確認**: `glxinfo`（Linux）や GPU-Z（Windows）

## 🔧 開発環境カスタマイズ

### Visual Studio 拡張機能（推奨）

1. **Visual Studio Spell Checker**: コメントのスペルチェック
2. **CodeMaid**: コード整理・フォーマット
3. **ResXManager**: リソース管理（多言語対応時）

### エディタ設定

**ツール** → **オプション** → **テキストエディター** → **C/C++**:

```
タブとインデント:
- タブサイズ: 4
- インデントサイズ: 4
- 空白の挿入: チェック

行番号:
- 行番号: チェック

書式設定:
- 全般 → セミコロン後の自動書式設定: チェック
```

### コンパイル最適化

**Release構成での最適化設定**:

**C/C++** → **最適化**:
```
最適化: 速度を最大にする (/O2)
インライン関数の展開: 任意の適切な (/Ob2)
組み込み関数を有効にする: はい (/Oi)
```

**リンカー** → **最適化**:
```
参照: はい (/OPT:REF)
COMDAT の圧縮: はい (/OPT:ICF)
```

## 📊 パフォーマンス測定

### フレームレート測定

デバッグビルドに追加:
```c
#ifdef _DEBUG
static int frame_count = 0;
static clock_t last_time = 0;

void measure_fps() {
    frame_count++;
    clock_t current_time = clock();
    
    if (current_time - last_time > CLOCKS_PER_SEC) {
        printf("FPS: %d\n", frame_count);
        frame_count = 0;
        last_time = current_time;
    }
}
#endif
```

### メモリ使用量監視

**診断ツール**の使用:
1. デバッグ実行中: デバッグ → ウィンドウ → 診断ツール
2. CPU使用率、メモリ使用量をリアルタイム監視
3. メモリリーク検出

## 🎯 配布用ビルド

### Release構成でのビルド

1. **構成変更**: Debug → **Release**
2. **プラットフォーム確認**: **x64**
3. **ソリューションのリビルド**

### 配布パッケージ作成

```powershell
# PowerShellスクリプト例
$releaseDir = "x64\Release"
$distDir = "PianoApp_Distribution"

# ディストリビューション フォルダ作成
New-Item -ItemType Directory -Path $distDir -Force

# 実行ファイル コピー
Copy-Item "$releaseDir\PianoApp.exe" $distDir

# ランタイム DLL コピー
Copy-Item "$releaseDir\freeglut.dll" $distDir
Copy-Item "$releaseDir\glew32.dll" $distDir

# データファイル コピー
Copy-Item "object" $distDir -Recurse
Copy-Item "textures" $distDir -Recurse
Copy-Item "timbres" $distDir -Recurse
Copy-Item "gakufu" $distDir -Recurse

# ドキュメント コピー
Copy-Item "README.md" $distDir

Write-Host "配布パッケージが $distDir に作成されました"
```

## 📚 追加リソース

### Visual Studio ドキュメント
- [C++ デスクトップ開発](https://docs.microsoft.com/ja-jp/cpp/)
- [NuGet パッケージ管理](https://docs.microsoft.com/ja-jp/nuget/)
- [Visual Studio デバッガー](https://docs.microsoft.com/ja-jp/visualstudio/debugger/)

### OpenGL リファレンス
- [OpenGL Registry](https://www.opengl.org/registry/)
- [GLUT Documentation](https://freeglut.sourceforge.net/docs/api.php)
- [GLEW](http://glew.sourceforge.net/)

### C言語 リファレンス
- [Microsoft C Runtime Library](https://docs.microsoft.com/ja-jp/cpp/c-runtime-library/)
- [C11 Standard](https://www.iso.org/standard/57853.html)

---

---

このガイドに従って環境を構築することで、3D電子ピアノアプリケーションを Visual Studio で快適に開発・デバッグできるようになります。問題が発生した場合は、エラーメッセージを確認し、該当する解決法を試してください。

**最終動作確認日**: 2025年8月26日  
**確認環境**: 
- Windows 11 Home (ビルド 26100, バージョン 24H2) + Panasonic Let'snote CFFV3-1
- Intel64 Family 6 Model 154 + Intel Iris Xe Graphics + Intel Wi-Fi 6E AX211
- Visual Studio 2022 Community Edition (17.14.36408.4)
- MSVC v143 (14.44.35207) + Windows SDK 10.0.26100
- MSBuild 17.14.18 + 開発者コマンドプロンプト 17.14.12

````
