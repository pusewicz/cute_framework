// swift-tools-version:5.9

import PackageDescription

let package = Package(
    name: "CuteRenderer",
    platforms: [
        .macOS(.v14),
        .iOS(.v17),
        .tvOS(.v17)
    ],
    products: [
        .library(name: "CuteRenderer", targets: ["CuteRenderer"]),
    ],
    targets: [
        // SDL3 system library
        .systemLibrary(
            name: "CSDL3",
            path: "Sources/CSDL3",
            pkgConfig: "sdl3",
            providers: [
                .brew(["sdl3"]),
                .apt(["libsdl3-dev"])
            ]
        ),
        // Main Swift renderer - thin wrapper over SDL3 GPU
        .target(
            name: "CuteRenderer",
            dependencies: ["CSDL3"],
            path: "Sources/CuteRenderer"
        ),
        .testTarget(
            name: "CuteRendererTests",
            dependencies: ["CuteRenderer"]
        ),
    ]
)
