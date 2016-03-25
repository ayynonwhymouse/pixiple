#include "shared.h"

#include "job.h"

#include "image.h"

ImagePair Job::get_next_pair() {
	std::unique_lock<std::mutex> ul{mutex};

	if (progress_current() == progress_total())
		return {nullptr, nullptr};

	while (images[index_major] == nullptr) {
		if (index_next_to_create < images.size()) {
			// create image
			auto i = index_next_to_create++;
			ul.unlock();
			auto image = std::make_shared<Image>(paths[i]);
			ul.lock();
			images[i] = image;
		} else {
			// no more images to create but allow other threads to finish creating images
			ul.unlock(); 
			ul.lock();
		}
	}

	auto index_minor_old = index_minor;
	auto index_major_old = index_major;

	if (index_minor == index_major) {
		index_major++;
		index_minor = 0;
	} else {
		index_minor++;
	}

	if (index_major == images.size())
		return {nullptr, nullptr};
	else
		return {images[index_minor_old], images[index_major_old]};
}

float Job::get_progress() const {
	std::lock_guard<std::mutex> lg{mutex};
	return static_cast<float>(progress_current()) / progress_total();
}

bool Job::is_completed() const {
	std::lock_guard<std::mutex> lg{mutex};
	return progress_current() == progress_total();
}

std::size_t Job::progress_current() const {
	return index_major * (1 + index_major) / 2 + index_minor;
}

std::size_t Job::progress_total() const {
	return paths.size() * (1 + paths.size()) / 2;
}
