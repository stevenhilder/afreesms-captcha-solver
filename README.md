# solver

![A sample CAPTCHA image](tests/images/532876.png)

Solves CAPTCHA images from the site **www.afreesms.com** -- part of the TrafficWorm SMS-bot project.

### Installation

Build solver on macOS or Linux with a C99-compatible compiler using the included Makefile.
The target system will need [ImageMagick](https://www.imagemagick.org/) with the **MagickCore** library available through `pkg-config`.

```bash
cd /path/to/afreesms-captcha-solver/
make
make test # optional
```

### Usage

```bash
./solve tests/images/532876.png # prints "532876"
```
