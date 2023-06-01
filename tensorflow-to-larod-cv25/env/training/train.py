"""
 * Copyright (C) 2021 Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
"""

""" train.py
    Instantiates a data generator and a model and trains the model.

    usage: train.py [-h] -i <path to dataset image dir> \
        -a <path to dataset annotation json-file>
"""

import argparse
import tensorflow as tf
from model import create_model
from utils import SimpleCOCODataGenerator as DataGenerator


def train(image_dir, annotation_path, train_epochs):
    """ Initiates a model and and trains it using a data generator. The model
        is then saved to the output path.

    Args:
        image_dir (str): Path to the directory holding the dataset images.
        annotation_path (str): Path to the dataset annotation json-file.
        train_epochs (int): Number of desired epochs to train the model.
    """
    person_car_indicator = create_model()
    person_car_indicator.compile(optimizer='adam', metrics=['binary_accuracy'],
                                 loss=['bce', 'bce'])
    person_car_indicator.summary()
    data_generator = DataGenerator(image_dir, annotation_path, batch_size=16)
    person_car_indicator.fit(data_generator, epochs=train_epochs)

    tf.saved_model.save(person_car_indicator, 'models/saved_model')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Train a basic model.')
    parser.add_argument('-i', '--images', type=str, required=True,
                        help='path to the directory containing training \
                        images')
    parser.add_argument('-a', '--annotations', type=str, required=True,
                        help='path to the .json-file containing COCO instance \
                        annotations')
    parser.add_argument('-e', '--training-epochs', type=int, default=8,
                        help='number of training epochs')

    args = parser.parse_args()

    train(args.images, args.annotations, args.training_epochs)

