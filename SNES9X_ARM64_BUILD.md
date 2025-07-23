# Snes9x EX Plus ARM64 最適化ビルドガイド

このガイドでは、Snes9x EX Plus を ARM64 アーキテクチャ専用で高速ビルドする方法を説明します。

## 🚀 主な最適化ポイント

### 1. アーキテクチャ特化
- **ARM64 専用ビルド**: 他のアーキテクチャ（armv6, armv7, x86, x86_64）を除外
- **NEON 最適化**: ARM64 の NEON SIMD 命令を活用
- **コンパイラ最適化**: `-march=armv8-a -mtune=cortex-a73 -O3 -flto`

### 2. ビルド時間短縮
- **並列ビルド**: `make -j$(nproc)` で全CPUコアを活用
- **ccache**: コンパイルキャッシュで再ビルド時間を大幅短縮
- **LTO (Link Time Optimization)**: リンク時最適化でパフォーマンス向上
- **効率的なキャッシュ戦略**: NDK、Gradle、Imagine SDK の段階的キャッシュ

### 3. CI/CD 最適化
- **条件付きトリガー**: 関連ファイル変更時のみビルド実行
- **アーティファクト管理**: タイムスタンプ付きAPKファイル名
- **詳細なビルドサマリー**: APKサイズ、ビルド時間、キャッシュ統計

## 📁 ファイル構成

```
.
├── .github/workflows/
│   └── snes9x-arm64-optimized.yml    # 最適化されたGitHub Actionsワークフロー
├── Snes9x/
│   ├── android-arm64-optimized.mk    # ARM64専用ビルド設定
│   └── (既存のSnes9xファイル)
├── build-snes9x-arm64.sh             # ローカルビルドスクリプト
└── SNES9X_ARM64_BUILD.md             # このファイル
```

## 🛠️ ローカルビルド方法

### 前提条件

```bash
# Ubuntu/Debian の場合
sudo apt-get update
sudo apt-get install -y \
    autoconf automake autopoint bash clang cmake \
    file gawk gettext git libtool libtool-bin \
    llvm make pkg-config unzip wget \
    build-essential ninja-build ccache openjdk-21-jdk
```

### 基本的なビルド

```bash
# リリースAPKをビルド
./build-snes9x-arm64.sh

# デバッグAPKをビルド
./build-snes9x-arm64.sh debug

# クリーンビルド
./build-snes9x-arm64.sh clean
```

### 環境変数での設定

```bash
# 特定のNDKバージョンを使用
NDK_VERSION=r26d ./build-snes9x-arm64.sh

# ビルド並列度を調整
MAKEFLAGS="-j8" ./build-snes9x-arm64.sh
```

## 🔧 GitHub Actions ワークフロー

### 手動実行

1. GitHub リポジトリの「Actions」タブに移動
2. 「Snes9x EX Plus ARM64 Optimized Build」を選択
3. 「Run workflow」をクリック
4. オプションでデバッグビルドを選択可能

### 自動実行

以下の条件で自動実行されます：
- `master` または `main` ブランチへのプッシュ
- 関連ディレクトリ（`Snes9x/`, `EmuFramework/`, `imagine/`）の変更
- プルリクエスト作成時

### ワークフロー最適化の詳細

#### キャッシュ戦略
```yaml
# NDKキャッシュ（約2GB、変更頻度低）
- uses: actions/cache@v4
  with:
    path: android-ndk-r27-beta2
    key: android-ndk-r27-beta2-linux

# Imagine SDKキャッシュ（ARM64専用）
- uses: actions/cache@v4
  with:
    path: imagine-sdk
    key: imagine-sdk-arm64-${{ hashFiles('imagine/**/*.mk') }}

# Gradleキャッシュ
- uses: actions/cache@v4
  with:
    path: |
      ~/.gradle/caches
      ~/.gradle/wrapper
      ~/.gradle/native
    key: gradle-${{ runner.os }}-snes9x-${{ hashFiles('**/*.gradle*') }}
```

#### ビルド最適化
```yaml
# ccache設定
- run: |
    ccache --set-config=max_size=2G
    ccache --set-config=compression=true

# Gradle最適化
- run: |
    echo "org.gradle.daemon=true" >> ~/.gradle/gradle.properties
    echo "org.gradle.parallel=true" >> ~/.gradle/gradle.properties
    echo "org.gradle.workers.max=4" >> ~/.gradle/gradle.properties
```

## 📊 パフォーマンス比較

### ビルド時間（推定）

| 設定 | 初回ビルド | キャッシュ有り | 改善率 |
|------|------------|----------------|--------|
| 従来版（全アーキテクチャ） | 45-60分 | 25-35分 | - |
| **ARM64最適化版** | **25-35分** | **8-15分** | **50-70%短縮** |

### APKサイズ

| アーキテクチャ | APKサイズ | 削減率 |
|----------------|-----------|--------|
| Universal APK | ~25MB | - |
| **ARM64専用APK** | **~8-12MB** | **50-65%削減** |

## 🔍 トラブルシューティング

### よくある問題

#### 1. NDKダウンロードエラー
```bash
# 手動でNDKをダウンロード
wget https://dl.google.com/android/repository/android-ndk-r27-beta2-linux.zip
unzip android-ndk-r27-beta2-linux.zip
```

#### 2. メモリ不足エラー
```bash
# 並列度を下げる
MAKEFLAGS="-j2" ./build-snes9x-arm64.sh
```

#### 3. 依存関係エラー
```bash
# 依存関係を再インストール
sudo apt-get update
sudo apt-get install --reinstall build-essential
```

### ログの確認

```bash
# ビルドログの詳細確認
./build-snes9x-arm64.sh 2>&1 | tee build.log

# エラー箇所の特定
grep -i error build.log
grep -i failed build.log
```

## 📈 さらなる最適化案

### 1. 分散ビルド
- **Buildbot/Jenkins**: 複数マシンでの並列ビルド
- **Docker**: 一貫したビルド環境の提供

### 2. プロファイリング最適化
- **PGO (Profile-Guided Optimization)**: 実行プロファイルに基づく最適化
- **カスタムコンパイラフラグ**: 特定デバイス向けチューニング

### 3. 継続的改善
- **ビルド時間計測**: 各ステップの詳細な時間測定
- **キャッシュヒット率**: キャッシュ効率の監視
- **APKサイズ追跡**: リリース間でのサイズ変化監視

## 📝 注意事項

1. **アーキテクチャ制限**: ARM64専用APKは古いデバイスでは動作しません
2. **NDKバージョン**: 最新のNDKを使用することを推奨します
3. **メモリ使用量**: 並列ビルド時は十分なRAM（8GB以上推奨）が必要です
4. **ディスク容量**: ビルド環境全体で約10-15GBの空き容量が必要です

## 🤝 貢献

ビルド最適化の改善案や問題報告は、以下の方法でお知らせください：

1. **Issues**: バグ報告や機能要望
2. **Pull Requests**: 改善案の提案
3. **Discussions**: 最適化手法の議論

---

**最終更新**: 2024年12月
**対象バージョン**: Snes9x EX Plus (emu-ex-plus-alpha)
**最適化レベル**: ARM64専用・高速ビルド対応