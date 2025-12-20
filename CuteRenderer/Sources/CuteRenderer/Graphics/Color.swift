import Foundation

/// A color with floating-point RGBA components (0.0 to 1.0).
///
/// `Color` is the primary type used for specifying colors throughout CuteRenderer.
/// Components are typically in the range 0.0 to 1.0, but values outside this range
/// are allowed for HDR rendering.
///
/// ## Topics
///
/// ### Creating Colors
/// - ``init(r:g:b:a:)``
/// - ``init(r:g:b:)``
/// - ``init(gray:alpha:)``
/// - ``init(hex:)``
/// - ``init(hex:alpha:)``
///
/// ### Predefined Colors
/// - ``white``
/// - ``black``
/// - ``red``
/// - ``green``
/// - ``blue``
/// - ``yellow``
/// - ``cyan``
/// - ``magenta``
/// - ``clear``
///
/// ### Color Operations
/// - ``lerp(to:t:)``
/// - ``premultiplied``
/// - ``withAlpha(_:)``
public struct Color: Hashable, Codable, Sendable {
    /// The red component (0.0 to 1.0).
    public var r: Float

    /// The green component (0.0 to 1.0).
    public var g: Float

    /// The blue component (0.0 to 1.0).
    public var b: Float

    /// The alpha (opacity) component (0.0 to 1.0).
    public var a: Float

    /// Creates a color with the specified RGBA components.
    @inlinable
    public init(r: Float, g: Float, b: Float, a: Float = 1.0) {
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    }

    /// Creates a grayscale color.
    @inlinable
    public init(gray: Float, alpha: Float = 1.0) {
        self.r = gray
        self.g = gray
        self.b = gray
        self.a = alpha
    }

    /// Creates a color from 8-bit RGBA values (0-255).
    @inlinable
    public init(r255: UInt8, g255: UInt8, b255: UInt8, a255: UInt8 = 255) {
        self.r = Float(r255) / 255.0
        self.g = Float(g255) / 255.0
        self.b = Float(b255) / 255.0
        self.a = Float(a255) / 255.0
    }

    /// Creates a color from a hexadecimal RGB value (e.g., 0xFF5500).
    @inlinable
    public init(hex: Int) {
        self.r = Float((hex >> 16) & 0xFF) / 255.0
        self.g = Float((hex >> 8) & 0xFF) / 255.0
        self.b = Float(hex & 0xFF) / 255.0
        self.a = 1.0
    }

    /// Creates a color from a hexadecimal RGB value with a separate alpha.
    @inlinable
    public init(hex: Int, alpha: Float) {
        self.r = Float((hex >> 16) & 0xFF) / 255.0
        self.g = Float((hex >> 8) & 0xFF) / 255.0
        self.b = Float(hex & 0xFF) / 255.0
        self.a = alpha
    }

    /// Creates a color from a hexadecimal RGBA value (e.g., 0xFF5500FF).
    @inlinable
    public init(hexRGBA: UInt32) {
        self.r = Float((hexRGBA >> 24) & 0xFF) / 255.0
        self.g = Float((hexRGBA >> 16) & 0xFF) / 255.0
        self.b = Float((hexRGBA >> 8) & 0xFF) / 255.0
        self.a = Float(hexRGBA & 0xFF) / 255.0
    }

    /// Creates a color from a hex string (e.g., "#FF5500" or "0xFF5500").
    public init?(hexString: String) {
        var hex = hexString.trimmingCharacters(in: .whitespacesAndNewlines).uppercased()

        if hex.hasPrefix("#") {
            hex = String(hex.dropFirst())
        } else if hex.hasPrefix("0X") {
            hex = String(hex.dropFirst(2))
        }

        var rgbValue: UInt64 = 0
        guard Scanner(string: hex).scanHexInt64(&rgbValue) else { return nil }

        if hex.count == 6 {
            self.init(hex: Int(rgbValue))
        } else if hex.count == 8 {
            self.init(hexRGBA: UInt32(rgbValue))
        } else {
            return nil
        }
    }

    // MARK: - Predefined Colors

    /// White (1, 1, 1, 1).
    public static let white = Color(r: 1, g: 1, b: 1, a: 1)

    /// Black (0, 0, 0, 1).
    public static let black = Color(r: 0, g: 0, b: 0, a: 1)

    /// Red (1, 0, 0, 1).
    public static let red = Color(r: 1, g: 0, b: 0, a: 1)

    /// Green (0, 1, 0, 1).
    public static let green = Color(r: 0, g: 1, b: 0, a: 1)

    /// Blue (0, 0, 1, 1).
    public static let blue = Color(r: 0, g: 0, b: 1, a: 1)

    /// Yellow (1, 1, 0, 1).
    public static let yellow = Color(r: 1, g: 1, b: 0, a: 1)

    /// Cyan (0, 1, 1, 1).
    public static let cyan = Color(r: 0, g: 1, b: 1, a: 1)

    /// Magenta (1, 0, 1, 1).
    public static let magenta = Color(r: 1, g: 0, b: 1, a: 1)

    /// Orange (1, 0.5, 0, 1).
    public static let orange = Color(r: 1, g: 0.5, b: 0, a: 1)

    /// Purple (0.5, 0, 0.5, 1).
    public static let purple = Color(r: 0.5, g: 0, b: 0.5, a: 1)

    /// Gray (0.5, 0.5, 0.5, 1).
    public static let gray = Color(r: 0.5, g: 0.5, b: 0.5, a: 1)

    /// Transparent black (0, 0, 0, 0).
    public static let clear = Color(r: 0, g: 0, b: 0, a: 0)

    // MARK: - Operations

    /// Returns this color with a new alpha value.
    @inlinable
    public func withAlpha(_ alpha: Float) -> Color {
        return Color(r: r, g: g, b: b, a: alpha)
    }

    /// Returns the color in premultiplied alpha form.
    ///
    /// Premultiplied alpha means the RGB components are multiplied by the alpha value.
    /// This is often more efficient for blending.
    @inlinable
    public var premultiplied: Color {
        return Color(r: r * a, g: g * a, b: b * a, a: a)
    }

    /// Linearly interpolates between this color and another.
    @inlinable
    public func lerp(to other: Color, t: Float) -> Color {
        return Color(
            r: r + (other.r - r) * t,
            g: g + (other.g - g) * t,
            b: b + (other.b - b) * t,
            a: a + (other.a - a) * t
        )
    }

    /// Returns the color with components clamped to the 0-1 range.
    @inlinable
    public var clamped: Color {
        return Color(
            r: max(0, min(1, r)),
            g: max(0, min(1, g)),
            b: max(0, min(1, b)),
            a: max(0, min(1, a))
        )
    }

    /// Converts to a Pixel (8-bit per channel).
    @inlinable
    public var pixel: Pixel {
        return Pixel(color: self)
    }
}

// MARK: - HSV Support

extension Color {
    /// Creates a color from HSV (hue, saturation, value) components.
    /// - Parameters:
    ///   - hue: The hue in the range 0-1 (maps to 0-360 degrees).
    ///   - saturation: The saturation in the range 0-1.
    ///   - value: The brightness value in the range 0-1.
    ///   - alpha: The alpha component.
    public init(hue: Float, saturation: Float, value: Float, alpha: Float = 1.0) {
        let h = hue * 6.0
        let c = value * saturation
        let x = c * (1 - abs(h.truncatingRemainder(dividingBy: 2) - 1))
        let m = value - c

        var r: Float = 0, g: Float = 0, b: Float = 0

        switch Int(h) % 6 {
        case 0: r = c; g = x; b = 0
        case 1: r = x; g = c; b = 0
        case 2: r = 0; g = c; b = x
        case 3: r = 0; g = x; b = c
        case 4: r = x; g = 0; b = c
        case 5: r = c; g = 0; b = x
        default: break
        }

        self.init(r: r + m, g: g + m, b: b + m, a: alpha)
    }

    /// Returns the HSV representation of this color.
    /// - Returns: A tuple of (hue, saturation, value) all in range 0-1.
    public var hsv: (hue: Float, saturation: Float, value: Float) {
        let maxC = max(r, max(g, b))
        let minC = min(r, min(g, b))
        let delta = maxC - minC

        var h: Float = 0
        if delta > 0 {
            if maxC == r {
                h = ((g - b) / delta).truncatingRemainder(dividingBy: 6)
            } else if maxC == g {
                h = (b - r) / delta + 2
            } else {
                h = (r - g) / delta + 4
            }
            h /= 6
            if h < 0 { h += 1 }
        }

        let s = maxC > 0 ? delta / maxC : 0
        return (h, s, maxC)
    }
}

// MARK: - Operators

extension Color {
    @inlinable
    public static func + (lhs: Color, rhs: Color) -> Color {
        return Color(r: lhs.r + rhs.r, g: lhs.g + rhs.g, b: lhs.b + rhs.b, a: lhs.a + rhs.a)
    }

    @inlinable
    public static func - (lhs: Color, rhs: Color) -> Color {
        return Color(r: lhs.r - rhs.r, g: lhs.g - rhs.g, b: lhs.b - rhs.b, a: lhs.a - rhs.a)
    }

    @inlinable
    public static func * (lhs: Color, rhs: Color) -> Color {
        return Color(r: lhs.r * rhs.r, g: lhs.g * rhs.g, b: lhs.b * rhs.b, a: lhs.a * rhs.a)
    }

    @inlinable
    public static func * (lhs: Color, rhs: Float) -> Color {
        return Color(r: lhs.r * rhs, g: lhs.g * rhs, b: lhs.b * rhs, a: lhs.a * rhs)
    }

    @inlinable
    public static func * (lhs: Float, rhs: Color) -> Color {
        return rhs * lhs
    }
}

// MARK: - CustomStringConvertible

extension Color: CustomStringConvertible {
    public var description: String {
        return "Color(r: \(r), g: \(g), b: \(b), a: \(a))"
    }
}

// MARK: - Pixel

/// A color with 8-bit RGBA components (0-255).
///
/// `Pixel` is used for image data and texture operations where
/// colors need to be represented as packed bytes.
public struct Pixel: Hashable, Codable, Sendable {
    /// The red component (0-255).
    public var r: UInt8

    /// The green component (0-255).
    public var g: UInt8

    /// The blue component (0-255).
    public var b: UInt8

    /// The alpha component (0-255).
    public var a: UInt8

    /// Creates a pixel with the specified RGBA components.
    @inlinable
    public init(r: UInt8, g: UInt8, b: UInt8, a: UInt8 = 255) {
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    }

    /// Creates a pixel from a Color.
    @inlinable
    public init(color: Color) {
        self.r = UInt8(max(0, min(255, color.r * 255)))
        self.g = UInt8(max(0, min(255, color.g * 255)))
        self.b = UInt8(max(0, min(255, color.b * 255)))
        self.a = UInt8(max(0, min(255, color.a * 255)))
    }

    /// Creates a pixel from a packed 32-bit RGBA value.
    @inlinable
    public init(rgba: UInt32) {
        self.r = UInt8((rgba >> 24) & 0xFF)
        self.g = UInt8((rgba >> 16) & 0xFF)
        self.b = UInt8((rgba >> 8) & 0xFF)
        self.a = UInt8(rgba & 0xFF)
    }

    /// White pixel.
    public static let white = Pixel(r: 255, g: 255, b: 255, a: 255)

    /// Black pixel.
    public static let black = Pixel(r: 0, g: 0, b: 0, a: 255)

    /// Transparent pixel.
    public static let clear = Pixel(r: 0, g: 0, b: 0, a: 0)

    /// The packed 32-bit RGBA value.
    @inlinable
    public var rgba: UInt32 {
        return (UInt32(r) << 24) | (UInt32(g) << 16) | (UInt32(b) << 8) | UInt32(a)
    }

    /// Converts to a Color (floating-point).
    @inlinable
    public var color: Color {
        return Color(r255: r, g255: g, b255: b, a255: a)
    }
}
