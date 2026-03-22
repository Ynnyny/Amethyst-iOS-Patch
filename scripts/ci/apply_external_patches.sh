#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

patch_mobileglues_framebuffer() {
  local file="Natives/external/MobileGlues/MobileGlues-cpp/gl/framebuffer.cpp"

  if [[ ! -f "$file" ]]; then
    return
  fi

  if grep -q "AMETHYST_MOBILEGLUES_FRAMEBUFFER_FIX" "$file"; then
    return
  fi

  python3 - "$file" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

old_bind = """void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    ensure_max_attachments();
    framebuffer_t& fbo = get_framebuffer(framebuffer);

    if (framebuffer == 0 && target != GL_READ_FRAMEBUFFER) {
        framebuffer = FSR1_Context::g_renderFBO;
        FSR1_Context::g_dirty = true;
    }

    if (target != GL_READ_FRAMEBUFFER) {
        set_gl_state_current_draw_fbo(framebuffer);
    }

    if (framebuffer != 0) {
        init_framebuffer(fbo);
    }
"""

new_bind = """void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    ensure_max_attachments();

    if (framebuffer == 0 && target != GL_READ_FRAMEBUFFER) {
        framebuffer = FSR1_Context::g_renderFBO;
        FSR1_Context::g_dirty = true;
    }

    framebuffer_t& fbo = get_framebuffer(framebuffer); // AMETHYST_MOBILEGLUES_FRAMEBUFFER_FIX

    if (target != GL_READ_FRAMEBUFFER) {
        set_gl_state_current_draw_fbo(framebuffer);
    }

    if (framebuffer != 0) {
        init_framebuffer(fbo);
    }
"""

old_drawbuffer = """void glDrawBuffer(GLenum buffer) {
    LOG()
    LOG_D(\"glDrawBuffer %d\", buffer)

    //    GLint currentFBO;
    //    GLES.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    if (current_draw_fbo == 0) {
"""

new_drawbuffer = """void glDrawBuffer(GLenum buffer) {
    LOG()
    LOG_D(\"glDrawBuffer %d\", buffer)

    ensure_max_attachments();
    if (current_draw_fbo != 0) {
        framebuffer_t& fbo = get_framebuffer(current_draw_fbo); // AMETHYST_MOBILEGLUES_FRAMEBUFFER_FIX
        init_framebuffer(fbo);
    }

    //    GLint currentFBO;
    //    GLES.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    if (current_draw_fbo == 0) {
"""

old_drawbuffers = """void glDrawBuffers(GLsizei n, const GLenum* bufs) {
    LOG()
    if (current_draw_fbo == 0) {
        GLES.glDrawBuffers(n, bufs);
        return;
    }

    framebuffer_t& fbo = framebuffers[current_draw_fbo];
"""

new_drawbuffers = """void glDrawBuffers(GLsizei n, const GLenum* bufs) {
    LOG()
    ensure_max_attachments();
    if (current_draw_fbo == 0) {
        GLES.glDrawBuffers(n, bufs);
        return;
    }

    framebuffer_t& fbo = get_framebuffer(current_draw_fbo); // AMETHYST_MOBILEGLUES_FRAMEBUFFER_FIX
    init_framebuffer(fbo);
"""

for old, new, label in [
    (old_bind, new_bind, "glBindFramebuffer"),
    (old_drawbuffer, new_drawbuffer, "glDrawBuffer"),
    (old_drawbuffers, new_drawbuffers, "glDrawBuffers"),
]:
    if old not in text:
        raise SystemExit(f"Expected block for {label} not found in {path}")
    text = text.replace(old, new, 1)

path.write_text(text)
PY

  echo "Applied local patch: $file"
}

patch_mobileglues_framebuffer
