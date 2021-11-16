#!/usr/bin/env python3
# testing scikit-image
import sys 
from skimage import data, io, filters

image = data.coins()
edges = filters.sobel(image)
# following does pauses as if in a effort to show
# but after a pause of not showing anything, it returns with no result.
io.imshow(edges)
# However, if you add the following, then it does work.
io.show()
