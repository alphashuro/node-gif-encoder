const GIFEncoder = require('./build/Debug/addon.node');
const fs = require('fs');
const gifFrames = require('gif-frames');
const sharp = require('sharp');

const image = './image.jpg';
const gif = './gif.gif';

const imagebuf = fs.readFileSync(image);
const gifbuf = fs.readFileSync(gif);

async function main() {

  const frames = await gifFrames({
    url: gifbuf,
    frames: 'all',
    outputType: 'png'
  });

  const firstFrameInfo = frames[0].frameInfo;
  const { width, height } = firstFrameInfo;

  const encoder = new GIFEncoder(width, height);

  encoder.start();
  encoder.setRepeat(0);
  encoder.setQuality(10);

  for (const frame of frames) {
    const { data } = frame.getImage();

    // overlay image
    // resize the base image to fit the overlay gif image dimensions
    // const image = await sharp(imagebuf)
    //   .resize({
    //     width: width,
    //     height: height,
    //     fit: sharp.fit.cover,
    //     position: sharp.strategy.attention
    //   })
    //   .raw()
    //   .composite([{
    //     input: data,
    //     blend: 'over',
    //     raw: { width, height, channels: 4 }
    //   }])
    //   .toBuffer();

    encoder.setFrameRate(100 / frame.frameInfo.delay);
    encoder.addFrame(data);
    console.log('frame added');
  }
  const result = encoder.finish();

  fs.writeFileSync('./result.gif', result);
  console.log('done');
}

main();
