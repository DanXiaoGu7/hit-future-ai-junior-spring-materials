"""
Hopfield network utilities for MNIST classification
"""

import torch


def to_bipolar(images, threshold=0.5):
    flat = images.view(images.size(0), -1)
    bipolar = torch.where(flat >= threshold, 1.0, -1.0)
    return bipolar.float()


def _run_kmeans(samples, num_clusters, num_iters, seed):
    if samples.size(0) <= num_clusters:
        return samples.clone()

    generator = torch.Generator(device=samples.device)
    generator.manual_seed(seed)
    initial_indices = torch.randperm(samples.size(0), generator=generator)[:num_clusters]
    centers = samples[initial_indices].clone()

    for _ in range(num_iters):
        distances = torch.cdist(samples, centers)
        assignments = distances.argmin(dim=1)
        new_centers = []

        for cluster_idx in range(num_clusters):
            cluster_samples = samples[assignments == cluster_idx]
            if cluster_samples.size(0) == 0:
                new_centers.append(centers[cluster_idx])
            else:
                new_centers.append(cluster_samples.mean(dim=0))

        new_centers = torch.stack(new_centers, dim=0)
        if torch.allclose(new_centers, centers):
            centers = new_centers
            break
        centers = new_centers

    return centers


def build_memory_patterns(dataloader, num_classes, num_memories_per_class, threshold, kmeans_iters, seed):
    class_samples = [[] for _ in range(num_classes)]

    for images, labels in dataloader:
        flat_images = images.view(images.size(0), -1)
        for class_idx in range(num_classes):
            mask = labels == class_idx
            if mask.any():
                class_samples[class_idx].append(flat_images[mask])

    memory_patterns = []
    memory_labels = []

    for class_idx in range(num_classes):
        samples = torch.cat(class_samples[class_idx], dim=0)
        if num_memories_per_class == 1:
            centers = samples.mean(dim=0, keepdim=True)
        else:
            centers = _run_kmeans(
                samples=samples,
                num_clusters=num_memories_per_class,
                num_iters=kmeans_iters,
                seed=seed + class_idx,
            )

        bipolar_centers = torch.where(centers >= threshold, 1.0, -1.0).float()
        memory_patterns.append(bipolar_centers)
        memory_labels.append(torch.full((bipolar_centers.size(0),), class_idx, dtype=torch.long))

    return torch.cat(memory_patterns, dim=0), torch.cat(memory_labels, dim=0)


def predict_by_memory_matching(states, memory_patterns, memory_labels):
    similarities = torch.matmul(states, memory_patterns.t())
    best_indices = similarities.argmax(dim=1)
    return memory_labels[best_indices]


class HopfieldNetwork:
    def __init__(self, num_units, device):
        self.num_units = num_units
        self.device = device
        self.weight_matrix = torch.zeros((num_units, num_units), dtype=torch.float32, device=device)

    def fit(self, patterns):
        patterns = patterns.float()
        self.weight_matrix = torch.matmul(patterns.t(), patterns) / self.num_units
        self.weight_matrix.fill_diagonal_(0.0)

    def load_weights(self, weight_matrix):
        self.weight_matrix = weight_matrix.float().to(self.device)

    def recall(self, states, max_steps):
        current_states = states.clone()
        steps_used = 0

        for step in range(1, max_steps + 1):
            updated_states = torch.sign(torch.matmul(current_states, self.weight_matrix))
            updated_states[updated_states == 0] = 1.0
            steps_used = step
            if torch.equal(updated_states, current_states):
                current_states = updated_states
                break
            current_states = updated_states

        return current_states, steps_used


@torch.no_grad()
def evaluate_model(network, dataloader, memory_patterns, memory_labels, threshold, recall_steps, device):
    total_correct = 0
    total_samples = 0
    total_recall_steps = 0.0

    memory_patterns = memory_patterns.float()
    memory_labels = memory_labels.long()

    for images, labels in dataloader:
        labels = labels.to(device)
        states = to_bipolar(images.to(device), threshold=threshold)
        steps_used = 0
        if recall_steps > 0:
            states, steps_used = network.recall(states, max_steps=recall_steps)
        predictions = predict_by_memory_matching(states, memory_patterns, memory_labels)

        total_correct += predictions.eq(labels).sum().item()
        total_samples += labels.size(0)
        total_recall_steps += steps_used * labels.size(0)

    accuracy = total_correct / total_samples
    avg_steps = total_recall_steps / total_samples
    return accuracy, avg_steps
