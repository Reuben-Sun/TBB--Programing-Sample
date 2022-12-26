#include <iostream>
#include <tbb/tbb.h>
#include "ImageLib.h"

using ImagePtr = std::shared_ptr<ImageLib::Image>;

ImagePtr applyGamma(ImagePtr image_ptr, double gamma){
    auto output_image_ptr = std::make_shared<ImageLib::Image>(image_ptr->name() + "_gamma",
                                                              ImageLib::IMAGE_WIDTH, ImageLib::IMAGE_HEIGHT);
    auto in_rows = image_ptr->rows();
    auto out_rows = output_image_ptr->rows();
    const int height = in_rows.size();
    const int width = in_rows[1] - in_rows[0];

    for(int i = 0; i < height; ++i){
        for(int j = 0; j < width; ++j){
            const ImageLib::Image::Pixel& p = in_rows[i][j];
            double v = 0.3 * p.bgra[2] + 0.59 * p.bgra[1] + 0.11 * p.bgra[0];
            double res = pow(v, gamma);
            if(res > ImageLib::MAX_BGR_VALUE){
                res = ImageLib::MAX_BGR_VALUE;
            }
            out_rows[i][j] = ImageLib::Image::Pixel(res, res, res);
        }
    }
    return output_image_ptr;
}

ImagePtr applyTint(ImagePtr image_ptr, const double *tints){
    auto output_image_ptr = std::make_shared<ImageLib::Image>(image_ptr->name() + "_tinted",
                                                              ImageLib::IMAGE_WIDTH, ImageLib::IMAGE_HEIGHT);
    auto in_rows = image_ptr->rows();
    auto out_rows = output_image_ptr->rows();
    const int height = in_rows.size();
    const int width = in_rows[1] - in_rows[0];

    for(int i = 0; i < height; ++i){
        for(int j = 0; j < width; ++j){
            const ImageLib::Image::Pixel& p = in_rows[i][j];
            std::uint8_t b = (double)p.bgra[0] + (ImageLib::MAX_BGR_VALUE - p.bgra[0]) * tints[0];
            std::uint8_t g = (double)p.bgra[0] + (ImageLib::MAX_BGR_VALUE - p.bgra[1]) * tints[1];
            std::uint8_t r = (double)p.bgra[0] + (ImageLib::MAX_BGR_VALUE - p.bgra[2]) * tints[2];
            out_rows[i][j] = ImageLib::Image::Pixel(
                    (b > ImageLib::MAX_BGR_VALUE) ? ImageLib::MAX_BGR_VALUE : b,
                    (g > ImageLib::MAX_BGR_VALUE) ? ImageLib::MAX_BGR_VALUE : g,
                    (r > ImageLib::MAX_BGR_VALUE) ? ImageLib::MAX_BGR_VALUE : r
                    );
        }
    }
    return output_image_ptr;
}

void writeImage(ImagePtr image_ptr){
    image_ptr->write((image_ptr->name() + ".bmp").c_str());
}

void fig1_7(const std::vector<ImagePtr>& image_vector){
    const double tint_array[] = {0.75, 0, 0};
    for(ImagePtr img: image_vector){
        writeImage(img);
        img = applyGamma(img, 1.4);
        writeImage(img);
        img = applyTint(img, tint_array);
        writeImage(img);
    }
}

int main() {
    std::vector<ImagePtr> image_vector;
    for(int i = 2000; i < 2000000; i *= 10){
        image_vector.push_back(ImageLib::makeFractalImage(i));
    }
    tbb::tick_count t0 = tbb::tick_count::now();
    fig1_7(image_vector);
    std::cout << "Time: " << (tbb::tick_count::now() - t0).seconds() << " seconds" << std::endl;
    return 0;
}
