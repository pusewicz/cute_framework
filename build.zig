const std = @import("std");
const Build = std.Build;

pub fn build(b: *Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const t = target.result;

    const physfs_dep = b.dependency("physfs", .{});

    const lib = b.addStaticLibrary(.{
        .name = "cute",
        .target = target,
        .optimize = optimize,
    });

    lib.addIncludePath(physfs_dep.path("src"));
    lib.addIncludePath(physfs_dep.path("extras"));
    lib.addIncludePath(b.path("include"));
    lib.addIncludePath(b.path("src"));
    lib.addIncludePath(b.path("libraries"));
    lib.addCSourceFiles(.{ .files = &cute_src_files });

    switch (t.os.tag) {
        .macos => {
            lib.addCSourceFiles(.{ .files = &cute_src_files });
            // lib.addCSourceFiles(.{
            //     .files = &objective_c_src_files,
            //     .flags = &.{"-fobjc-arc"},
            // });
            lib.linkFramework("OpenGL");
            lib.linkFramework("Metal");
            lib.linkFramework("CoreVideo");
            lib.linkFramework("Cocoa");
            lib.linkFramework("IOKit");
            lib.linkFramework("ForceFeedback");
            lib.linkFramework("Carbon");
            lib.linkFramework("CoreAudio");
            lib.linkFramework("AudioToolbox");
            lib.linkFramework("AVFoundation");
            lib.linkFramework("Foundation");
        },
        else => {},
    }

    var files = std.ArrayList([]const u8).init(b.allocator);
    defer files.deinit();

    lib.addCSourceFiles(.{ .files = files.toOwnedSlice() catch @panic("OOM") });

    lib.linkLibCpp();
    lib.linkLibrary(addPhysfs(b, target, optimize));

    b.installArtifact(lib);
}

fn addPhysfs(
    b: *Build,
    target: Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *Build.Step.Compile {
    const dep = b.dependency("physfs", .{});
    const root = dep.path(".");
    const lib = b.addStaticLibrary(.{
        .name = "physfs",
        .target = target,
        .optimize = optimize,
    });
    var files = std.ArrayList([]const u8).init(b.allocator);
    files.appendSlice(&.{
        "src/physfs.c",
        "src/physfs_archiver_7z.c",
        "src/physfs_archiver_dir.c",
        "src/physfs_archiver_grp.c",
        "src/physfs_archiver_hog.c",
        "src/physfs_archiver_iso9660.c",
        "src/physfs_archiver_mvl.c",
        "src/physfs_archiver_qpak.c",
        "src/physfs_archiver_slb.c",
        "src/physfs_archiver_unpacked.c",
        "src/physfs_archiver_vdf.c",
        "src/physfs_archiver_wad.c",
        "src/physfs_archiver_zip.c",
        "src/physfs_byteorder.c",
        "src/physfs_unicode.c",
        "src/physfs_platform_posix.c",
        "src/physfs_platform_unix.c",
        "src/physfs_platform_windows.c",
        "src/physfs_platform_haiku.cpp",
        "src/physfs_platform_android.c",
    }) catch @panic("OOM");
    if (target.result.isDarwin()) {
        files.append("src/physfs_platform_apple.m") catch @panic("OOM");
    }
    lib.addCSourceFiles(.{
        .root = root,
        .files = files.toOwnedSlice() catch @panic("OOM"),
    });
    lib.linkLibCpp();
    return lib;
}

const cute_src_files = [_][]const u8{
    "src/cute_alloc.cpp",
    "src/cute_array.cpp",
    // "src/cute_audio.cpp",
    // "src/cute_clipboard.cpp",
    // "src/cute_multithreading.cpp",
    // "src/cute_file_system.cpp",
    // "src/cute_input.cpp",
    // "src/cute_time.cpp",
    "src/cute_version.cpp",
    "src/cute_json.cpp",
    "src/cute_base64.cpp",
    "src/cute_hashtable.cpp",
    // "src/cute_string.cpp",
    "src/cute_math.cpp",
    // "src/cute_draw.cpp",
    // "src/cute_image.cpp",
    // "src/cute_graphics.cpp",
    // "src/cute_aseprite_cache.cpp",
    // "src/cute_png_cache.cpp",
    // "src/cute_https.cpp",
    // "src/cute_joypad.cpp",
    // "src/cute_symbol.cpp",
    // "src/cute_sprite.cpp",
    // "src/cute_coroutine.cpp",
    "src/cute_networking.cpp",
    "src/cute_guid.cpp",
    "src/cute_alloc.cpp",
    // "src/cute_result.cpp",
    "src/cute_noise.cpp",
    // "src/cute_imgui.cpp",

    "src/internal/yyjson.c",

    "libraries/cimgui/imgui/imgui.cpp",
    "libraries/cimgui/imgui/imgui_demo.cpp",
    "libraries/cimgui/imgui/imgui_draw.cpp",
    "libraries/cimgui/imgui/imgui_tables.cpp",
    "libraries/cimgui/imgui/imgui_widgets.cpp",

    "libraries/cimgui/cimgui.cpp",
};
