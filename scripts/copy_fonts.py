# Post-build script: copies resources/fonts/ to build output directory
# so the release exe can find them at <exe_dir>/resources/fonts/

Import("env")
import os
import shutil

def copy_fonts_to_build(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")

    src_fonts = os.path.join(project_dir, "resources", "fonts")
    dst_fonts = os.path.join(build_dir, "resources", "fonts")

    if not os.path.exists(src_fonts):
        print(f"[copy_fonts] WARNING: Source fonts directory not found: {src_fonts}")
        return

    # Remove old copy to avoid stale files
    if os.path.exists(dst_fonts):
        shutil.rmtree(dst_fonts)

    shutil.copytree(src_fonts, dst_fonts)
    lff_count = len([f for f in os.listdir(dst_fonts) if f.endswith(".lff")])
    print(f"[copy_fonts] Copied {lff_count} font files to {dst_fonts}")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.exe", copy_fonts_to_build)
