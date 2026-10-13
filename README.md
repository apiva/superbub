# Super Bub Contest

### What is this?

This is a modern port of the Net Yaroze classic game *Super Bub*, created by Alex Herbert in 1998. "Modern port" means that the original code has been adapted to modern compilers and platforms, and can be built and run natively (as of 2026).

### Why?

I - the creator of this port - was 13 when I found Super Bub on a PlayStation Magazine demo disc in 1998. I played it with my sister for a ton of hours, and I have many fond memories of it. I didn't want this game to be forgotten.

### Where did you get the source code?

A few years ago I found the source code, along with the original assets (in Net Yaroze's proprietary `TIM` and `VAB` formats) in the [Internet Archive](https://archive.org/details/super-bub).

### What does Alex Herbert think about this?

This is the saddest part - I have no idea. I've been searching for ways to contact Alex for years, to no avail. If you know him, please let him know of this repository, and ask him to contat me.

Since I was able to find the source code online, I assume it's been Alex himself to publish it a long time ago for everyone to see, so I think he'd have liked this project.

### What is the license of this?

The original source code just says *Copyright (c) 1998 Alex Herbert*, with no license. So, all of the code must still be considered property of Alex.

As far as my own modifications go, consider them in the public domain.

### How did you do this port?

I tried to preserve Alex's original code as much as possible. I only replaced the calls to Net Yaroze libraries with my own implementations, that use [SDL](https://github.com/libsdl-org/SDL). You should be able to build this on every platform that SDL supports.

Also, the original content files (`.tim` files, etc.) are used as-is. This repo contains readers for such files.

### What was the Net Yaroze?

The [Net Yaroze](https://en.wikipedia.org/wiki/Net_Yaroze) was Sony's stripped-down devkit for the first PlayStation, aimed at hobbyist developers. In PSM (PlayStation Magazine) demo discs you could find a bunch a games developed with it, some of which quite sophisticated.