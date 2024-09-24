# Face recognition example

## Introduction

The task of face recognition, as understood in the current example, is to locate the face in a given picture among a database of faces. In a ZK-relevant setting, the prover is in the possession of either the picture of the face, or the database of faces, while the verifier has a digest of it. The other piece is known to both parties. The prover wants to convince the verifier that the face is, or is not an element in the database.

In the current example, we support the scenarios where the picture of a face is known to only the prover, while the database is known to both parties. Our code implements the steps described in [Adam Geitgey's write-up on face recognition via machine learning](https://medium.com/@ageitgey/machine-learning-is-fun-part-4-modern-face-recognition-with-deep-learning-c3cffc121d78). According to this write-up, there are four steps: _Location_, _Alignment_, _Encoding_, and _Comparison_, all of which have been implemented in the _[dlib](https://github.com/davisking/dlib)_ library for machine learning. Below, we discuss each of these steps, and their implementation in ZK-SecreC.

## Steps of face recognition

### Location

The first step in a face recognition procedure is to locate the faces on the given picture. For certain ZK use-cases, e.g. for the one where the prover claims to hold a picture with the face of someone in the database, this task is trivial (from the perspective of the statement being proved), consisting of the prover inputting the location(s) of the face(s) in the picture. For some other use-cases, the face location has to be run in a manner that the verifier can verify. For example, if the prover wants to convince the verifier that a particular picture does not contain a particular individual, or any individual from some database, then all faces on the picture have to be located, together with evidence that there are no more faces.

Face location can be done by computing _Histograms of Oriented Gradients_ (HOG) for the input picture at various scalings, and comparing it to a publicly known HOG pattern. Our ZK-SecreC implementation straightforwardly, and even somewhat tediously follows the described method; particularly its implementation in the _dlib_ library. The implementation has to compute the HOGs for all areas of the picture; for proving that we have handled all faces in the picture, we cannot leave some areas unprocessed.

### Alignment

The second step in face recognition is to warp the face just found, such that its main features (_landmarks_) are placed at "standard" locations in the image. At the same time, the image also has to be scaled to a "standard" size. This step is crucial for the following _Encoding_ step to perform satisfactorily. Experimental evidence suggests that this transformation can be done with a _similarity transform_ (i.e. a linear transform that does not shear the image); the alignment step has to find the parameters of this transform.

Our approach is based on a method in _dlib_ library, which we have reimplemented in ZK-SecreC. The method tries to find the five __anchor points__ of the face: the centroids of the eyes, the corners of the mouth, and the tip of the nose. Having found these five points, the method constructs the best similarity transform that moves these points to the "standard" locations. In order to find the locations of these five anchor points, the method first positions them to an initial position (corresponding to some "average" face), and then applies a sequence of _random forests_ to them. Each random forest, based on the intensities of the current anchor points and of the 800 _feature points_ defined relatively to the anchor points, suggest a shift for each anchor point. The forests are applied one after another, interspersed with the calculation of the new locations and intensities of anchor and feature points. The _dlib_ library includes a suitable pre-trained sequence of 15 random forests, each consisting of 500 complete binary trees with 16 leaves.

After applying the 15 random forests, using the colors of the feature points, the method has presumably found the anchor points of the current facial image. It constructs a similarity transformation that maps these anchor points to standard positions. The inverse of this transformation (i.e. we actually found the inverse transformation) is applied to each pixel of the output image, finding the pixel(s) in the original image that determine the color of that pixel in the output image. The color is determined by linearly interpolating the colors of the pixels of the original image.


Our implementation in ZK-SecreC largely follows the same steps. The method for finding the best similarity transformation has been largely ported from _dlib_, while the computation of the singular value decomposition (SVD) for a 2×2 matrix (of covariance between the current and shifted positions of anchor points) is by [an explicit formula](https://www.researchgate.net/publication/263580188_Closed_Form_SVD_Solutions_for_2_x_2_Matrices_-_Rev_2).

Perhaps the most interesting detail not yet covered above is the determination of the colors of feature pixels. After applying the first random forest, the locations of these pixels are private, because the shift of the anchor points and the original locations of feature points is determined by the facial image. We compute the coordinates of the pixels using the linear transformation that we have found, and then use the techniques of oblivious RAM to find the colors of these pixels. Namely, we load the entire facial image into a store at the start of the computation. The colors are needed each time we start processing a random forest, at which time we read the store by the computed coordinates of feature pixels.

### Encoding

The purpose of the _encoding_ step is to turn the located and aligned image into a low-dimensional (e.g. of length 128) vector of real numbers, such that the images of the same face map into close vectors, while the images of different faces map into distant vectors. A possible approach is to train a neural network to perform this task. The weights of the trained network can be used to build an encoder.

We have implemented in ZK-SecreC a residual neural network (RNN) from _dlib_; the description of this network can be found in lines 45-71 of the file `dnn_face_recognition_ex.cpp` in the `examples` folder of _dlib_. The given RNN transforms _colour_ images of size 150×150 pixels into a vector of 128 real numbers. The network has 132 layers, including 29 convolutional layers, 29 affine layers, 29 ReLU layers, one max-pool layer, and one fully connected layer for producing the output. The network has [ca. 5.6 million weights](http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2), most of which are used in convolutional layers. We have instrumented the _dlib_ implementation to extract the weights and encode them as verifier's (or public) input.

Our implementation is heavily optimized, in order to speed up both the generation and the evaluation of a circuit.

### Comparison

Having converted all the images (both the one on the picture, as well as the database of facial images) to relatively short vectors, the comparison step consists of computing the distances between these vectors, and comparing them to a threshold.

We do not have a separate implementation of the comparison step in this example.

## Face recognition in ZK-SecreC

We describe our implementation below. The implementation only uses the features supported by our integration with the Mac'n'Cheese back-end.

### Location

The HOG method has been implemented in `FaceLocator.zksc`, by the functions `init_face_locator` and `apply_face_locator`. The first of them reads in the description of the public HOG, and other parameters that describe when a match is sufficiently close to be classified as a face. The second of them takes as input a monochrome image, and computes (in verified manner) the bounds of a rectangle containing a face. The function `apply_face_locator` will fail (i.e. cause `assert(false)` to be called) if the input image does not contain exactly one face.

The description of the public HOG is given as part of public and verifier's inputs, located in `FaceLocation_public.json` and `FaceLocation_instance.json`. We have elected to make the numeric parameters of the HOG as part of verifier's input, while the "indexing" parameters of HOG are given as public input.

An example of calling the face locator is given in `FaceLocation.zksc`. The program there reads in an image as prover's input; a possible input is given in `FaceLocation.witness.json`, while the size of that image is given as part of the public input. The `main` function in this ZK-SecreC file calls the functions for face location. It does not do anything "interesting" with the obtained location, but simply prints the obtained bounding box to the console.

### Alignment

The Sequence of Random Forests method has been implemented in `FaceAligner.zksc`, with the similarity transform being computed by the code in `SimilarityTransform.zksc`. The function `init_face_aligner` reads in the descriptions of all the trees in all the forests, with numeric parameters as part of verifier's input (to be read from `FaceAlignment_instance.json`), while the indices are public inputs (in `FaceAlignment_public.json`). The function `apply_face_aligner` takes as input the initial image of a face (as a monochrome picture), and returns the aligned and scaled image.

An example of calling the face aligner is given in `FaceAlignment.zksc`. The `main` function in this program reads an image (of public size) and a bounding box from prover's input (in `FaceAlignment_witness.zksc`) and calls the face aligner. The aligned image is simply printed on the console.

### Encoding

The particular Residual Neural Network and its execution have been implemented in `ResidualNetwork.zksc`. In this file, certain conversions between data types have been declared as external functions; the implementations of these functions are located in `resnet_externs.rs`. Similarly to previous steps, the function `init_resnet` reads the weights of the network from verifier's input (in `resnet_instance.json`), while the function `apply_resnet` applies it to an image of size 150×150 pixels. Differently from previous steps, the image now consists of three color channels, not just one. The output of `apply_resnet` is a vector of 128 fixed-point numbers.

An example of invoking the RNN is given in the file `resnet.zksc`, where the `main` function reads in a picture from prover's input (examples are given in files `resnet_witness.json` and `resnet_witness[123].json`), applies the RNN to it, and prints the resulting vector of numbers on the console.

### Combination

This folder also contains ZK-SecreC programs that combine some of the steps described above.

* `FaceRecognition.zksc` combines the alignment and encoding steps. It also contains the comparison step in the end, where the vectors corresponding to the facial images' database are a part of verifier's input. The public, verifier's, and prover's inputs are given in various `FaceRecognition_*.json` files, with `FaceRecognition_witness_x_y.json` encoding the _y_-th picture of the _x_-th person.
* `FaceRecognitionFull.zksc` additionally runs the location step.

Note that while the implementation of the alignment step returns a monochrome image, the encoding step expects a colour image as the input. In the combination of steps, we simply triplicate the aligned image. We have found that such combination gives good results.

## Supporting scripts

The included Python scripts `generate_witness.py` and `generate_witness_frame.py` can be used to convert a **greyscale** image file to a JSON-file that can be used as input to our face recognition example. The first of these scripts creates a `witness.json` file that contains the values of pixels of the image, as well as a `public.json` file that contains the dimensions of the image. The second of these scripts additionally takes the final desired size (both the width and the length; i.e. the encoded image will be square-shaped) of the image, puts the original image to the middle, and records the location of the original image also in the `witness.json` file.

In our experiments, we have used the images from the [Labeled Faces in the Wild](https://vis-www.cs.umass.edu/lfw/) dataset; mostly from its [cropped](https://conradsanderson.id.au/lfwcrop/) version.

