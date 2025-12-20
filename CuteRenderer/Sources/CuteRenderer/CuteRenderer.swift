// CuteRenderer - A Swift 2D Renderer using SDL3 GPU
//
// This library provides a thin Swift wrapper over SDL3 GPU for 2D rendering
// with sprite and shape drawing capabilities, inspired by Cute Framework.
//
// Features:
// - High-level 2D drawing API for shapes
// - Sprite rendering with animation support
// - Direct access to SDL3 GPU for custom rendering

@_exported import CSDL3

// MARK: - Version

/// The CuteRenderer version.
public enum CuteRendererVersion {
    public static let major = 1
    public static let minor = 0
    public static let patch = 0
    public static var string: String { "\(major).\(minor).\(patch)" }
}
