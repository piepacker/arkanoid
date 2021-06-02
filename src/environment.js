mergeInto(LibraryManager.library, {
	js_env_controller_info_reset: function() {
		for (let p = 0; p < Module.env_controller_info.length; p++) {
			Module.env_controller_info[p].clear()
		}
	},

	js_env_controller_info_set: function (port, id, name_ptr, name_len) {
		if (port >= Module.env_controller_info.length) {
			return
		}

		// Convert C string to js string
		let name = '';
		const view = new Uint8Array(wasmMemory.buffer, name_ptr, name_len);
		for (let i = 0; i < name_len; i++) {
			name += String.fromCharCode(view[i]);
		}

		Module.env_controller_info[port].set(name, id)
	},
});
