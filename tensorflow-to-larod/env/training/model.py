"""
 * Copyright (C) 2021 Axis Communications AB, Lund, Sweden
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


def _residual_block(x, n_filters, strides=1):
    """ Produces a residual convolutional block as seen in
        https://en.wikipedia.org/wiki/Residual_neural_network

    Args:
        x: The input tensor
        n_filters (int): The number of filters for the convolutional layers

    Returns:
        x: The output tensor
    """
    shortcut = x

    x = Conv2D(n_filters, 3, strides=strides, padding='same')(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    x = Conv2D(n_filters, 3,  padding='same')(x)
    x = BatchNormalization()(x)

    shortcut = Conv2D(n_filters, 1, strides=strides, padding='same')(shortcut)
    shortcut = BatchNormalization()(shortcut)

    x = Add()([shortcut, x])
    x = Activation('relu')(x)
    return x


def create_model(blocks=4, n_filters=16):
    """ Defines and instantiates a model.

    Args:
        blocks (int): The number of residual blocks to apply. Each such block
            halves the width and height of its input.
        n_filters (int): The number of filters in the first residual block.
            This number is doubled for each residual block.

    Returns:
        model: The instantiated but uncompiled model.
    """
    img_in = Input(shape=(256, 256, 3))

    x = img_in
    for block_index in range(blocks):
        x = _residual_block(x, n_filters * 2 ** block_index)
        x = _residual_block(x, n_filters * 2 ** (block_index + 1), strides=2)

    # Global max pooling is not supported on the Edge TPU yet
    # but this is an equivalent procedure made of supported operations
    side_size = K.int_shape(x)[-2]
    x = MaxPooling2D(side_size)(x)
    x = Flatten()(x)

    x = Dense(64, activation='relu')(x)
    person_pred = Dense(1, activation='sigmoid', name='A_person_pred')(x)
    car_pred = Dense(1, activation='sigmoid', name='B_car_pred')(x)

    return Model(img_in, [person_pred, car_pred], name='person_car_indicator')
