const GIFEncoder = require("../build/Debug/addon.node");
const fs = require("fs");
const gifFrames = require("gif-frames");
const sharp = require("sharp");

const gif = "example/png/out.gif";
const gifbuf = fs.readFileSync(gif);

async function main() {
  const frames = await gifFrames({
    url: gifbuf,
    frames: "all",
    outputType: "png",
  });

  // const firstFrameInfo = frames[0].frameInfo;
  // const { width, height } = firstFrameInfo;

  const encoder = new GIFEncoder(854, 480);

  encoder.start();
  // encoder.setRepeat(0);
  // encoder.setQuality(10);

  // encoder.setFrameRate(1);

  encoder.addFrame(frames[0].getImage().data);
  console.log(`frame 1 added`);

  // encoder.addFrame(image2);
  // console.log(`frame 2 added`);

  // encoder.addFrame(image3);
  // console.log(`frame 3 added`);

  // await Promise.all(
  // frames.map((f, i) => {
  // const { data } = frame.getImage();

  // overlay image;
  // resize the base image to fit the overlay gif image dimensions;
  // const image = await sharp(imagebuf)
  //   .resize({
  //     width: width,
  //     height: height,
  //     fit: sharp.fit.cover,
  //     position: sharp.strategy.attention,
  //   })
  //   .raw()
  //   .composite([
  //     {
  //       input: data,
  //       blend: "over",
  //       raw: { width, height, channels: 4 },
  //     },
  //   ])
  //   .toBuffer();

  //     encoder.setFrameRate(100 / frame.frameInfo.delay);
  //     encoder.addFrame(data);
  //     console.log(`frame ${i} added`);
  //   })
  // );
  // for (const frame of frames) {

  // }
  const result = encoder.finish();

  fs.writeFileSync("example/result.gif", result);
  console.log("done");
}

main();
