//
// Created by Filip Lux on 27.11.16.
//

#include "ConvLayer.h"


double vectorDotProduct(double* a, double* b, int &dim_a, int &dim_b, const int &steps_i,const int &steps_j, const int input_depth) {
    double ans = 0;
    for (int i = 0; i <= steps_i; i++) {
        for (int j = 0; j <= steps_j; j++) {
            for ( int k = 0; k < input_depth; k++) {
                ans += a[i * dim_a + j + k * dim_a * dim_a] * b[i * dim_b + j + k * dim_b * dim_b];
            }
        }
    }
    return ans;
}


double dotProduct(double* input, double* ddot, int &dim, const int &steps_i,const int &steps_j) {
    double ans = 0;
    int all_d;
    for (int i = 0; i <= steps_i; i++) {
        for (int j = 0; j <= steps_j; j++) {
            all_d = i * dim + j;
            ans += input[all_d] * ddot[all_d];
        }
    }
    return ans;
}

double backPropDotProduct(double* ddot, double* w, int &dim_ddot, int &dim_w,
                            const int &steps_i, const int &steps_j,
                            const int &wn, const int &depth) {

    double ans = 0;
    int ddot_d;
    int w_d;
    for (int i = 0; i <= steps_i; i++) {
        for (int j = 0; j <= steps_j; j++) {
            ddot_d = i * dim_ddot + j;
            w_d = (steps_i - i) * dim_w + steps_j - j;
            for (int k = 0; k < depth; k++) {
                ans += ddot[ddot_d + depth*k] * w[w_d + k*wn];
            }

        }
    }
    return ans;
}



ConvLayer::ConvLayer(int filter_dim, int stroke, int filters, int in_dim, int in_depth, double* in) {

    //parameters of the input
    input_depth = in_depth;
    input_dim = in_dim;
    input = in; //pole vektoru

    //parameters of the output
    dim = input_dim;
    depth = filters;
    n = dim * dim;

    //parameters of the filters
    w_dim = filter_dim;
    wn = w_dim * w_dim;  //number of weights in one layer
    s = stroke;



    w = new double[wn*depth*input_depth];
    for (int i = 0; i < wn*depth*input_depth; i++) {
        w[i] = fRand(INIT_MIN, INIT_MAX);
    }

    bias = new double[depth];
    for (int j = 0; j < depth; j++) {
        bias[j] = fRand(INIT_MIN, INIT_MAX);

    }

    out = new double[n*depth];
    ddot = new double[n*depth]; //predat do horni vrstvy
}

ConvLayer::ConvLayer(int filter_dim, int stroke, int filters, Layer* lower) {
    //conection to lower layer
    down = lower;

    //parameters of the input
    input_depth = down->depth;
    input_dim = down->dim;
    input = down->out;
    down_ddot = down->ddot;

    //parameters of the output
    dim = input_dim;
    depth = filters;
    n = dim * dim;

    //parameters of the filters
    w_dim = filter_dim;
    wn = w_dim * w_dim;          //number of weights in one layer
    s = stroke;


    w = new double[wn*depth*input_depth];
    for (int i = 0; i < wn*depth*input_depth; i++) {
        w[i] = fRand(INIT_MIN, INIT_MAX);
    }

    bias = new double[depth];
    for (int j = 0; j < depth; j++) {
        bias[j] = fRand(INIT_MIN, INIT_MAX);
    }

    out = new double[n*depth];
    ddot = new double[n*depth]; //predat do horni vrstvy
}



void ConvLayer::forward_layer() {

    const int diff = w_dim/2;
    double sum;

    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {

            const int out_diff = i * dim + j;

            const int ii = std::max(0, i-diff);
            const int jj = std::max(0, j-diff);
            const int di = std::min(dim-1, i + diff) - ii;
            const int dj = std::min(dim-1, j + diff) - jj;
            const int in_diff = (ii * dim + jj);                            //begin of the input
            const int w_diff = (diff - i + ii) * w_dim + (diff - j + jj);   //begin of the w

            for (int d = 0; d < depth; d++) {

                sum = bias[d] +
                        vectorDotProduct(&input[in_diff], &w[w_diff + d*wn], dim, w_dim, di, dj, input_depth);
                //out[out_diff + d*n] = ( sum > 0 ) ? sum : 0;         //ReLu
                out[out_diff + d*n] = sigma(sum);                   //sigmoid
            }
        }
    }

}


void ConvLayer::backProp_layer() {  //modify lower ddot array



    const int diff = w_dim/2;

    /**
    for ( int i = 0; i < n * depth; i++) {      //derivate of ReLU
        ddot[i] = (ddot[i] > 0) ? 1 : 0;
    }
     */

    for (int i = 0; i < n*depth; i++) {         //derivate of sigmoidal
        ddot[i] *= out[i] * (1-out[i]);
    }

    if (down != NULL)
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {

                const int down_ddot_diff = i * dim + j;

                const int ii = std::max(0, i-diff);
                const int jj = std::max(0, j-diff);
                const int di = std::min(dim-1, i + diff) - ii;
                const int dj = std::min(dim-1, j + diff) - jj;
                const int ddot_diff = (ii * dim + jj);                             //begin of the input
                const int w_diff = (diff - i + ii) * w_dim + (diff - j + jj);       //begin of the w

                for (int d = 0; d < input_depth; d++) {

                    down_ddot[down_ddot_diff + n*d] = backPropDotProduct(&ddot[ddot_diff], &w[w_diff + d*wn*depth], dim, w_dim, di, dj, wn, depth);

                }
            }
        }
}

void ConvLayer::learn() {

    const int diff = w_dim/2;

    for (int i = 0; i < w_dim; i++) {
        for (int j = 0; j < w_dim; j++) {

            const int ii = std::max(0,i - diff);
            const int jj = std::max(0,j - diff);
            const int di = std::min(dim-1, i + diff) - ii;
            const int dj = std::min(dim-1, j + diff) - jj;
            const int input_diff = ii * dim + jj;
            const int ddot_diff = (w_dim - ii - di +1) * dim + w_dim - jj - dj +1;
            const int w_diff = w_dim*i + j;

            for (int d = 0; d < depth; d++) {
                for (int k = 0; k < input_depth; k++) {
                    w[w_diff + d * wn * depth + k * wn] +=
                            dotProduct(&input[input_diff + k*n], &ddot[ddot_diff + d*n], dim, di, dj) * LR;

                }
            }
        }
    }

}


void ConvLayer::print() {
    //not implemented
}


void ConvLayer::update_input(double* in) {
    input = in;
}


ConvLayer::~ConvLayer() {
    delete []bias;
    delete []ddot;
    delete []out;
    delete []w;
};