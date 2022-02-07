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

""" utils.py

    Holds utility functions for the model training process, specifically the
    data generator.
"""

from tensorflow.keras.utils import Sequence
import numpy as np
import json
import os

from PIL import Image


class SimpleCOCODataGenerator(Sequence):
    """ A data generator which reads data on the MS COCO format and
        reprocesses it to simply output whether a certain class exists in
        a given image.
    """
    def __init__(self, samples_dir, annotation_path, width=480, height=270,
                 batch_size=16, shuffle=True, balance=True):
        """ Initializes the data generator.

        Args:
            samples_dir (str): The path to the directory in which the dataset
                images are located.
            annotation_path (str): The path to the annotations json-file
            data_shape (tuple of ints, optional): The resolution to output
                images on.
            batch_size (int, optional): The number of samples per batch.
            shuffle (bool, optional): Whether to shuffle the dataset sample
                order after each epoch.
            balance (bool, optional): Whether to oversample the data in order
                reduce the effect of imbalanced classes
        """
        print('Creating data generator..')
        self.samples_dir = samples_dir
        annotations = json.load(open(annotation_path, 'r'))
        self.annotations = self._reprocess_annotations(annotations)
        self.width = width
        self.height = height
        self.batch_size = batch_size
        self.shuffle = shuffle
        self.balance = balance
        self.on_epoch_end()

    def __len__(self):
        """ Returns the number of data batches available to this generator.
        """
        return int(len(self.annotations) / self.batch_size)

    def __getitem__(self, index):
        """ Returns a specific batch of data.

        Args:
            index (int): The index of the data batch to return
        """
        batch_indices = self.indices[index * self.batch_size:
                                     (index + 1) * self.batch_size]
        batch_annotations = self.annotations[batch_indices]
        X, y = self._generate_batch(batch_annotations)
        return X, y

    def on_epoch_end(self, weights=[1, 1, 1, 3]):
        """ Function executed at the end of an epoch. Shuffles the dataset
            sample order if self.shuffle is True and balances the data
            by performing oversampling. A bit of a hacky balancing solution due
            to Tensorflow r2.3 not supporting class_weight for multiple outputs

            Args:
                weights (int array): An integer array where each element
                    corresponds to the sample weighting of a class
        """
        self.indices = np.arange(len(self.annotations))
        if self.balance:
            indices_set = set(self.indices)
            not_has_person = indices_set - self.sample_classes['has_person']
            not_has_car = indices_set - self.sample_classes['has_car']
            classes = [not_has_person,
                       self.sample_classes['has_person'],
                       not_has_car,
                       self.sample_classes['has_car']]
            samples_per_class = np.max([len(c) for c in classes])
            self.indices = np.concatenate([np.random.choice(list(c),
               size=int(weights[idx] * samples_per_class)) for idx, c
               in enumerate(classes)])
        if self.shuffle is True:
            np.random.shuffle(self.indices)

    def _reprocess_annotations(self, annotations):
        """ Extracts information from the given dataset which is relevant to
            the model to train.

        Args:
            annotations: A dict with MS COCO annotations.

        Returns:
            A refined set of annotations which hold values directly related
            to if there are 1) person(s) in the image, 2) vehicle(s) in the
            image. Additionally, the samples are only kept if the
            corresponding images 1) exist and 2) are on the RGB format.
        """
        print('Reprocessing annotations..')
        has_person = set()
        has_car = set()

        # The car class is in fact car, bus and truck
        car_labels = set([3, 6, 8])
        for annotation in annotations['annotations']:
            # The person category
            if annotation['category_id'] == 1:
                has_person.add(annotation['image_id'])
            # The car / 4-wheeled vehicle category
            elif annotation['category_id'] in car_labels:
                has_car.add(annotation['image_id'])

        self.sample_classes = {'has_car': set(), 'has_person': set()}
        processed_annotations = []
        for image in annotations['images']:
            sample = {'file_name': image['file_name'],
                      'id': image['id'],
                      'has_car': image['id'] in has_car,
                      'has_person': image['id'] in has_person}
            img_path = os.path.join(self.samples_dir, image['file_name'])
            file_exists = os.path.exists(img_path)

            if file_exists and Image.open(img_path).mode == 'RGB':
                sample_idx = len(processed_annotations)
                if image['id'] in has_person:
                    self.sample_classes['has_person'].add(sample_idx)
                if image['id'] in has_car:
                    self.sample_classes['has_car'].add(sample_idx)
                processed_annotations.append(sample)
        return np.array(processed_annotations)

    def _generate_batch(self, batch_annotations):
        """ Produces a batch of data from a given list of modified MS COCO
            annotations.

        Args:
            batch_annotations: A dict with MS COCO annotations that have been
                modified with the has_person and has_car-fields.

        Returns:
            A batch of data as an (X, y) tuple
        """
        X = np.zeros((self.batch_size, self.height, self.width, 3),
                     dtype=np.float32)
        y_person = np.zeros((self.batch_size, 1), dtype=np.float32)
        y_car = np.zeros((self.batch_size, 1), dtype=np.float32)

        for i, annotation in enumerate(batch_annotations):
            img_path = os.path.join(self.samples_dir, annotation['file_name'])
            img = Image.open(img_path).resize((self.width, self.height))

            # Horizontal flipping with p=0.5
            if np.random.random() >= 0.5:
                img = img.transpose(Image.FLIP_LEFT_RIGHT)

            X[i, ] = np.array(img)
            y_person[i, ] = annotation['has_person']
            y_car[i, ] = annotation['has_car']

        X /= 255.
        return X, (y_person, y_car)
