const GIFEncoder = require('./build/Release/addon.node')

const encoder = new GIFEncoder(200, 200)

console.log("hello ", encoder.test())
