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

""" train.py
    Trains and exports a model on a specified, COCO-style dataset. The process is
    done in three steps: first, the model is trained for 8 epochs. Next, the model is fine-tuned with
    quantization-aware training for 4 epochs. Finally, the model's quantization nodes are converted
    to their evaluation configured equivalent and the model is exported in this state.

    usage: train.py [-h] -i <path to dataset image dir> \
        -a <path to dataset annotation json-file>
"""

import argparse
import tensorflow as tf
from model import create_model
from utils import SimpleCOCODataGenerator as DataGenerator


def export_evaluation_model(quantized_model_path, frozen_graph_path, model_configuration):
    """ Initiates a model, loads trained, quantized weights and configures
        the model for prediction. Finally, the configured model is exported.

    Args:
        quantized_model_path (str): Path pointing to the quantized, trained model model.
        evaluation_model_path (str): Path to output the final evaluation model to.
        model_configuration (dict): Key-word arguments to the model creation function
    """
    eval_graph = tf.Graph()
    eval_sess = tf.Session(graph=eval_graph)
    tf.keras.backend.set_session(eval_sess)

    with eval_graph.as_default():
        tf.keras.backend.set_learning_phase(0)

        # Initiate the model
        model = create_model(**model_configuration)

        # Add evaluation-time quantization nodes to the model
        tf.contrib.quantize.create_eval_graph(eval_graph)

        # Load quantized model parameter
        tf.train.Saver().restore(eval_sess, quantized_model_path)

        # Prepare the frozen graph for evaluation
        frozen_graph_def = eval_graph.as_graph_def()
        frozen_graph_def = tf.graph_util.remove_training_nodes(frozen_graph_def)
        frozen_graph_def = tf.graph_util.convert_variables_to_constants(
        eval_sess,
        frozen_graph_def,
        [t.op.name for t in model.output]
        )

        # Save the frozen graph
        with open(frozen_graph_path, 'wb') as f:
            f.write(frozen_graph_def.SerializeToString())
        model.summary()


def quantize_and_finetune_model(data_generator, weights_path, quantized_model_path, model_configuration, tune_epochs):
    """ Initiates a model, loads trained weights and performs quantization aware training
        by finetuning the trained model with quantization nodes. The quantized model
        model are then saved to quantized_model_path.

    Args:
        data_generator (str): A data generator for supplying training data.
        weights_path (str): Path pointing to the trained model weights.
        quantized_model_path (str): Path to output the quantized, trained model model to.
        model_configuration (dict): Key-word arguments to the model creation function.
        train_epochs (int): Number of desired epochs to finetune the model.
    """
    # Create a new session and graph for the quantization-aware training
    finetune_graph = tf.Graph()
    finetune_sess = tf.Session(graph=finetune_graph)
    tf.keras.backend.set_session(finetune_sess)

    with finetune_graph.as_default():
        tf.keras.backend.set_learning_phase(1)
        tf.compat.v1.disable_eager_execution()
        # Initiate a new model
        model = create_model(**model_configuration)

        # Load the trained weights into the model
        tf.train.Saver().restore(finetune_sess, weights_path)

        # Add quantization nodes to the graph and initialize the new quantization variables
        model_variables = set(tf.all_variables())
        tf.contrib.quantize.create_training_graph(finetune_graph)

        # Compile and finetune the model
        finetune_sess.run(tf.variables_initializer(set(tf.all_variables()) - model_variables))
        model.compile(optimizer='adam', metrics=['binary_accuracy'],
                      loss=['binary_crossentropy', 'binary_crossentropy'])

        model.fit(data_generator, epochs=tune_epochs, workers=4)
        tf.train.Saver().save(finetune_sess, quantized_model_path)


def train_model(data_generator, trained_model_path, model_configuration, train_epochs):
    """ Initiates a model and trains it using a data generator. The model
        weights are then saved to the output path.

    Args:
        data_generator (str): A data generator for supplying training data.
        weights_path (str): Path to output the trained model weights to.
        model_configuration (dict): Key-word arguments to the model creation function.
        train_epochs (int): Number of desired epochs to train the model.
    """
    # Create a new session and graph for the training
    train_graph = tf.Graph()
    train_sess = tf.Session(graph=train_graph)
    tf.keras.backend.set_session(train_sess)

    with train_graph.as_default():
        # Initiate the model
        model = create_model(**model_configuration)

        # Compile and train the model
        model.compile(optimizer='adam', metrics=['binary_accuracy'],
                      loss=['binary_crossentropy', 'binary_crossentropy'])

        model.summary()
        model.fit(data_generator, epochs=train_epochs, workers=4)

        # Save model
        tf.train.Saver().save(train_sess, trained_model_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Train a basic model.')
    parser.add_argument('-i', '--images', type=str, required=True,
                        help='path to the directory containing training \
                        images')
    parser.add_argument('-a', '--annotations', type=str, required=True,
                        help='path to the .json-file containing COCO instance \
                        annotations')
    parser.add_argument('--input-width', type=int, default=480,
                        help='The width of the model\'s input image')
    parser.add_argument('--input-height', type=int, default=270,
                        help='The height of the model\'s input image')
    parser.add_argument('-e', '--training-epochs', type=int, default=8,
                        help='number of training epochs')
    parser.add_argument('-t', '--tuning-epochs', type=int, default=4,
                        help='number of fine-tuning epochs')
    args = parser.parse_args()

    print('Using TensorFlow version: {}'.format(tf.__version__))
    data_generator = DataGenerator(args.images, args.annotations, batch_size=8,
                                   width=args.input_width, height=args.input_height)

    trained_model_path = '/env/models/fp32_model/model'
    quantized_model_path = '/env/models/qat_model/model'
    final_frozen_graph_path = '/env/models/trained_model.pb'
    train_epochs = args.training_epochs
    tune_epochs = args.tuning_epochs

    model_configuration = {'input_shape': (args.input_height, args.input_width, 3),
                           'n_blocks': 5, 'n_filters': 16}

    train_model(data_generator, trained_model_path, model_configuration, train_epochs)
    quantize_and_finetune_model(data_generator, trained_model_path,
                                quantized_model_path, model_configuration, tune_epochs)
    export_evaluation_model(quantized_model_path, final_frozen_graph_path,
                            model_configuration)
