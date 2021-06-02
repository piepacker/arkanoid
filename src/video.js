mergeInto(LibraryManager.library, {
  $VideoBridge: {
    getHeapFromFormat: function (depth) {
      switch (depth) {
        default:
          throw new Error("Invalid depth (" + depth + ")");

        case 1:
          return HEAPU8;
        case 0:
        case 2:
          return HEAPU16;
      }
    },

    castPointerToData: function (pointer, heap, count) {
      var start = pointer / heap.BYTES_PER_ELEMENT;
      var end = start + count;

      // RETRO_PIXEL_FORMAT_XRGB8888
      if (Module.video.format == 1) {
        return new Uint8ClampedArray(heap.subarray(start, end));
      }
      return heap.subarray(start, end);
    },
  },

  js_video_set_size: function (width, height, pitch) {
    Module.video.setInputSize(width, height, pitch);
  },

  js_video_set_pixel_format: function (format) {
    Module.video.setPixelFormat(format);
  },

  js_video_set_data__deps: ["$VideoBridge"],
  js_video_set_data: function (dataPointer) {
    var heap = VideoBridge.getHeapFromFormat(Module.video.format);

    Module.video.setInputData(
      VideoBridge.castPointerToData(
        dataPointer,
        heap,
        Module.video.height * Module.video.pitch
      )
    );
  },

  js_video_render: function () {
    Module.video.render();
  },

  // this is meant to refer to emulator internal framerate (emux_fps).
  // FIXME: clarification! We must always clearly differentiate between the emulator's
  // internal framerate (emux_fps), also known as the emu framebuffer framerate, and the
  // host's framerate (host_fps), also known as host vsync or framebuffer presentation fps.
  js_video_set_fps: function (fps) {
    Module.video.fps = fps;
  },
});
