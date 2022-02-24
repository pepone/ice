//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

const _ModuleRegistry = require("../Ice/ModuleRegistry").Ice._ModuleRegistry;

require("../Glacier2/PermissionsVerifier");
require("../Glacier2/Router");
require("../Glacier2/Session");
require("../Glacier2/SSLInfo");

module.exports.Glacier2 = _ModuleRegistry.module("Glacier2");
