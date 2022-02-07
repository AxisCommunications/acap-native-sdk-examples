"""
 * Copyright (C) 2020 Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
"""


""" model.py
    Defines the model's structure and configuration.
"""
from tensorflow.keras.layers import *
from tensorflow.keras.models import Model
from tensorflow.keras import backend as K


def _residual_block(x, n_filters, strides):
    """ Produces a residual convolutional block as seen in
        https://en.wikipedia.org/wiki/Residual_neural_network

    Args:
        x: The input tensor
        n_filters (int): The number of filters for the convolutional layers
        strides (int): The downsampling convolutional layers' strides along
                       height and width

    Returns:
        x: The output tensor
    """
    shortcut = x

    x = Conv2D(n_filters, 3, strides=strides, padding='same', use_bias=False)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    x = Conv2D(n_filters, 3,  padding='same', use_bias=False)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    shortcut = Conv2D(n_filters, 1, strides=strides, padding='same', use_bias=False)(shortcut)
    shortcut = BatchNormalization()(shortcut)
    shortcut = Activation('relu')(shortcut)

    x = Add()([shortcut, x])
    return x


def create_model(n_blocks=4, n_filters=16, input_shape=(480, 270, 3)):
    """ Defines and instantiates a model.

    Args:
        n_blocks (int): The number of residual blocks to apply. Each such block
            halves the width and height of its input.
        n_filters (int): The number of filters in the first residual block.
            This number is doubled for each residual block.
        input_shape ((int, int, int)): The shape of the input image

    Returns:
        model: The instantiated but uncompiled model.
    """
    img_in = Input(shape=input_shape, name='image_input')

    x = img_in
    for block_index in range(n_blocks):
        x = _residual_block(x, n_filters * 2 ** block_index, strides=1)
        x = _residual_block(x, n_filters * 2 ** (block_index + 1), strides=2)

    side_h = K.int_shape(x)[-2]
    side_w = K.int_shape(x)[-3]
    x = MaxPooling2D((side_w, side_h))(x)
    x = Flatten()(x)

    x = Dense(64, activation='relu')(x)
    person_pred = Dense(1, activation='sigmoid', name='A_person_pred')(x)
    car_pred = Dense(1, activation='sigmoid', name='B_car_pred')(x)

    return Model(img_in, [person_pred, car_pred], name='person_car_indicator')
