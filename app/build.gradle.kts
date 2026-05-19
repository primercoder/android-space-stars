plugins {
    id("com.android.application")
}

android {
    namespace = "org.trisolar"
    compileSdk = 34

    defaultConfig {
        applicationId = "org.trisolar"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments(
                    "-DANDROID_PLATFORM=android-26",
                    "-DANDROID_STL=c++_shared"
                )
                cppFlags("-std=c++20")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.28.3"
        }
    }
}
