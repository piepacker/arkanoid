mergeInto(LibraryManager.library, {
  js_input_poll: function () {},

  js_input_state: function (port, id) {
    return Module.input_user_state[port][id];
  },

  js_analogL_input_state: function (port, id) {
      return Module.input_analogL_user_state[port][id];
  },

  js_analogR_input_state: function (port, index, id) {
      return Module.input_analogR_user_state[port][id];
  },
});
