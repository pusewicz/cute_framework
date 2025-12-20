// swift-tools-version:5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "CuteRenderer",
    platforms: [
        .macOS(.v14),
        .iOS(.v17),
        .tvOS(.v17),
        .visionOS(.v1)
    ],
    products: [
        .library(
            name: "CuteRenderer",
            targets: ["CuteRenderer"]),
    ],
    dependencies: [
        // SDL3 would be added here as a dependency in a real project
    ],
    targets: [
        // C bridging module for SDL3 GPU interop
        .target(
            name: "CuteRendererC",
            dependencies: [],
            path: "Sources/CuteRendererC",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include")
            ]
        ),
        // Main Swift implementation
        .target(
            name: "CuteRenderer",
            dependencies: ["CuteRendererC"],
            path: "Sources/CuteRenderer",
            swiftSettings: [
                .enableExperimentalFeature("StrictConcurrency")
            ]
        ),
        .testTarget(
            name: "CuteRendererTests",
            dependencies: ["CuteRenderer"]),
    ]
)
