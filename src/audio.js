mergeInto(LibraryManager.library, {
  $AudioBridge: {
    castPointerToData: function (pointer, heap, count) {
      var start = pointer / heap.BYTES_PER_ELEMENT;
      var end = start + count;

      return heap.subarray(start, end);
    },
  },

  js_audio_configure: function (rate, channels) {
    Module.audio.configure(rate, channels);
  },

  js_audio_push_sample_batch__deps: ["$AudioBridge"],
  js_audio_push_sample_batch: function (samplesPointer, count) {
    Module.audio.pushSampleBatch(
      AudioBridge.castPointerToData(samplesPointer, HEAP16, count * 2)
    );
  },
});
