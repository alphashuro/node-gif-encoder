const GIFEncoder = require("../build/Release/addon.node");
const GIFEncoderJS = require("gifencoder")
const fs = require("fs");
const gifFrames = require("gif-frames");
const sharp = require("sharp");
const {
  performance,
  PerformanceObserver
} = require('perf_hooks');

const gifbuf = fs.readFileSync("example/gif.gif");
const imagebuf = fs.readFileSync("example/image.jpg")

main();

async function main() {
  const frames = await gifFrames({
    url: gifbuf,
    frames: "all",
    outputType: "png",
  });

  const firstFrameInfo = frames[0].frameInfo;
  const { width, height } = firstFrameInfo;

  const images = await Promise.all(frames.map(overlayImage))

  const cppstart = process.hrtime()
  const result1 = await overlayGif({ images, width, height })
  const cppend = process.hrtime(cppstart)


  const jsstart = process.hrtime()
  const result2 = await overlayGifJs({ images, width, height })
  const jsend = process.hrtime(jsstart)
  
  fs.writeFileSync("example/resultcpp.gif", result1);
  fs.writeFileSync("example/resultjs.gif", result2);
  
  console.log("done");
  console.info('Execution time (cpp): %ds %dms', cppend[0], cppend[1] / 1000000)
  console.info('Execution time (js): %ds %dms', jsend[0], jsend[1] / 1000000)
}

function overlayGif({ images, width, height }) {
  const encoder = new GIFEncoder(width, height);

  encoder.start();
  encoder.setRepeat(0);
  encoder.setQuality(10);
  encoder.setFrameRate(1);

  for (const { image,  delay } of images) {
    encoder.setFrameRate(100 / delay);
    encoder.addFrame(image);
  }

  return encoder.finish();
}

function overlayGifJs({ images, width, height }) {
  return new Promise( (resolve, reject) => {
    const encoder = new GIFEncoderJS(width, height);

    const stream = encoder.createReadStream()
    var buffers = []; 
    stream.on("data", function(data) { 
      buffers.push(data); 
    }); 
    stream.on("end", function() { 
      resolve(Buffer.concat(buffers));
    })
  
    encoder.start();
    encoder.setRepeat(0);
    encoder.setQuality(10);
    encoder.setFrameRate(1);
  
    for (const { image,  delay } of images) {
      encoder.setFrameRate(100 / delay);
      encoder.addFrame(image);
    }
  
    return encoder.finish(); 
  })
  
}

async function overlayImage (frame) {
  const { data } = frame.getImage();
  const { width, height } = frame.frameInfo;

    // overlay image;
    // resize the base image to fit the overlay gif image dimensions;
    const image = await sharp(imagebuf)
      .resize({
        width: width,
        height: height,
        fit: sharp.fit.cover,
        position: sharp.strategy.attention,
      })
      .raw()
      .composite([
        {
          input: data,
          blend: "over",
          raw: { width, height, channels: 4 },
        },
      ])
      .toBuffer();

    return { delay: frame.frameInfo.delay, image }
}