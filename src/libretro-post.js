Module["input_user_state"] = [{}, {}, {}, {}, {}];
Module["input_analogR_user_state"] = [{}, {}, {}, {}, {}];
Module["input_analogL_user_state"] = [{}, {}, {}, {}, {}];
Module["env_controller_info"] = [new Map(), new Map(),new Map(), new Map(),new Map(), new Map(),new Map(), new Map()];

Module["inputState"] = function(port, id) {
  return Module["input_user_state"][port][id];
}

Module["onRuntimeInitialized"] = function () {
  this.frontendLoop = this.cwrap("frontend_loop", null, ["number"]);
  this.frontendIterate = this.cwrap("frontend_iterate", null, []);
  this.frontendSkipFrame = this.cwrap("frontend_skip_frame", null, ["number"]);
  this.frontendSetFast = this.cwrap("frontend_set_fast", null, []);
  this.frontendUnsetFast = this.cwrap("frontend_unset_fast", null, []);
  this.frontendLoadGame = this.cwrap("frontend_load_game", "number", ["number"]);
  this.frontendUnloadGame = this.cwrap("frontend_unload_game", "number", []);
  this.frontendGetState = this.cwrap("frontend_get_state", "number", ["number", "number"]);
  this.frontendSetState = this.cwrap("frontend_set_state", "number", ["number", "number"]);
  this.frontendReset = this.cwrap("frontend_reset", "number", []);
  this.frontendGetMemorySize = this.cwrap("frontend_get_memory_size", "number", ["number"]);
  this.frontendGetMemoryData = this.cwrap("frontend_get_memory_data", "number", ["number"]);
  this.frontendSetPaused = this.cwrap("frontend_set_paused", null, ["boolean"]);
  this.frontendSetOptions = this.cwrap("frontend_set_options", null, ["number", "number"])
  this.inputSetControllerPortDevice = this.cwrap("input_set_controller_port_device", null, ["number", "number"])

  this.memorySaveRAM = 0;
};

Module["setControllerPortDevice"] = function (port, device) {
  this.inputSetControllerPortDevice(port, device);
}

Module["setOptions"] = function (key, value) {
  var stackPointer = this.stackSave();

  var keyPointer = this.allocate(
    this.intArrayFromString(key),
    "i8",
    this.ALLOC_STACK
  );
  var valuePointer = this.allocate(
    this.intArrayFromString(value),
    "i8",
    this.ALLOC_STACK
  );

  this.frontendSetOptions(keyPointer, valuePointer);

  this.stackRestore(stackPointer);
}

Module["loadGame"] = function (gamePath) {
  var stackPointer = this.stackSave();

  var gamePathPointer = this.allocate(
    this.intArrayFromString(gamePath),
    "i8",
    this.ALLOC_STACK
  );

  var result = this.frontendLoadGame(gamePathPointer);

  this.stackRestore(stackPointer);

  if (result < 0)
    throw new Error("Game load failed - the core returned " + result);
};

Module["loadArrayBuffer"] = function (arrayBuffer, gamePath) {
  var stackPointer = this.stackSave();

  var gamePathPointer = this.allocate(this.intArrayFromString(gamePath), 'i8', this.ALLOC_STACK);
  FS.writeFile(gamePath, new Uint8Array(arrayBuffer), { encoding : 'binary' });

  this.frontendLoadGame(gamePathPointer);

  this.stackRestore(stackPointer);
}

Module["unloadGame"] = function () {
  return this.frontendUnloadGame();
}

Module["setState"] = function (arrayBuffer) {
    if (!arrayBuffer) return this.resetState();

    let size = arrayBuffer.byteLength
    let heap_ptr = this._malloc(size)

    writeArrayToMemory(new Uint8Array(arrayBuffer), heap_ptr);

    var result = this.frontendSetState(heap_ptr, size);

    this._free(heap_ptr)

    if (result < 0)
        throw new Error(
            "Cannot set state at this time - the core returned " + result
        );

    return this;
};

Module["getState"] = function () {
  var stackPointer = this.stackSave();

  var emDataPtr = this.allocate([0], "*", this.ALLOC_STACK);
  var emSizePtr = this.allocate([0], "i32", this.ALLOC_STACK);

  var result = this.frontendGetState(emDataPtr, emSizePtr);

  var emdata = this.getValue(emDataPtr, "*");
  var emsize = this.getValue(emSizePtr, "i32");

  this.stackRestore(stackPointer);

  if (result < 0)
    throw new Error(
      "Cannot get state at this time - the core returned " + result
    );

  return this.HEAPU8.buffer.slice(emdata, emdata + emsize);
};

Module["iterate"] = function () {
  this.frontendIterate();
};

Module["loop"] = function (fps) {
  this.frontendLoop(fps);
};

Module["skip_frame"] = function (frames) {
  this.frontendSkipFrame(frames);
};

Module["setFast"] = function () {
  this.frontendSetFast();
};

Module["unsetFast"] = function () {
  this.frontendUnsetFast();
};

Module["reset"] = function () {
  this.frontendReset();
};

Module["getSRAM"] = function () {
  var stackPointer = this.stackSave();

  var size = this.frontendGetMemorySize(this.memorySaveRAM);
  var ptr = this.frontendGetMemoryData(this.memorySaveRAM);

  this.stackRestore(stackPointer);

  return this.HEAPU8.buffer.slice(ptr, ptr + size);
};

Module["setPaused"] = function (paused) {
  this.frontendSetPaused(paused);
};

Module["setSRAM"] = function (arrayBuffer) {
  var stackPointer = this.stackSave();

  var size = this.frontendGetMemorySize(this.memorySaveRAM);
  var ptr = this.frontendGetMemoryData(this.memorySaveRAM);

  if (arrayBuffer.byteLength === size)
    Module.writeArrayToMemory(arrayBuffer, ptr);

  this.stackRestore(stackPointer);
};
