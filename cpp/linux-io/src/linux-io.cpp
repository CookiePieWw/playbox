#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libaio.h>
#include <liburing.h>
#include <linux/fs.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

// Common configuration
struct Config {
	std::string filename = "testfile.dat";
	size_t file_size = 1ULL << 30; // 1GB
	size_t block_size = 4096;
	size_t num_operations = 1000;
	bool verbose = false;
};

// Base benchmark interface
class IOMethod {
  public:
	virtual ~IOMethod() = default;
	virtual bool init(const Config &config) = 0;
	virtual bool run_read() = 0;
	virtual bool run_write() = 0;
	virtual void cleanup() = 0;
	virtual std::string name() const = 0;
};

// Utility functions
class Utils {
  public:
	static bool create_test_file(const std::string &filename, size_t size) {
		int fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (fd < 0) {
			perror("open");
			return false;
		}

		char *buffer = new char[4096];
		memset(buffer, 'A', 4096);

		size_t remaining = size;
		while (remaining > 0) {
			size_t to_write = std::min(remaining, (size_t)4096);
			if (write(fd, buffer, to_write) != (ssize_t)to_write) {
				perror("write");
				close(fd);
				delete[] buffer;
				return false;
			}
			remaining -= to_write;
		}

		close(fd);
		delete[] buffer;
		return true;
	}

	static bool remove_file(const std::string &filename) {
		return unlink(filename.c_str()) == 0;
	}

	template <typename Func> static double benchmark(Func &&func) {
		auto start = std::chrono::high_resolution_clock::now();
		if (!func()) {
			return -1.0;
		}
		auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	}
};

// Traditional buffered IO
class BufferedIO : public IOMethod {
  private:
	Config config_;
	int fd_ = -1;
	char *buffer_ = nullptr;

  public:
	bool init(const Config &config) override {
		config_ = config;
		buffer_ = new char[config_.block_size];
		return true;
	}

	bool run_read() override {
		fd_ = open(config_.filename.c_str(), O_RDONLY);
		if (fd_ < 0) {
			perror("open read");
			return false;
		}

		for (size_t i = 0; i < config_.num_operations; ++i) {
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			if (lseek(fd_, offset, SEEK_SET) < 0) {
				perror("lseek");
				return false;
			}
			if (read(fd_, buffer_, config_.block_size) !=
				(ssize_t)config_.block_size) {
				perror("read");
				return false;
			}
		}
		return true;
	}

	bool run_write() override {
		fd_ = open(config_.filename.c_str(), O_WRONLY);
		if (fd_ < 0) {
			perror("open write");
			return false;
		}

		memset(buffer_, 'B', config_.block_size);
		for (size_t i = 0; i < config_.num_operations; ++i) {
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			if (lseek(fd_, offset, SEEK_SET) < 0) {
				perror("lseek");
				return false;
			}
			if (write(fd_, buffer_, config_.block_size) !=
				(ssize_t)config_.block_size) {
				perror("write");
				return false;
			}
		}
		return true;
	}

	void cleanup() override {
		if (fd_ >= 0) {
			close(fd_);
			fd_ = -1;
		}
		delete[] buffer_;
		buffer_ = nullptr;
	}

	std::string name() const override { return "Buffered IO"; }
};

// Direct IO (O_DIRECT)
class DirectIO : public IOMethod {
  private:
	Config config_;
	int fd_ = -1;
	char *buffer_ = nullptr;

  public:
	bool init(const Config &config) override {
		config_ = config;
		if (posix_memalign((void **)&buffer_, 512, config_.block_size) != 0) {
			perror("posix_memalign");
			return false;
		}
		return true;
	}

	bool run_read() override {
		fd_ = open(config_.filename.c_str(), O_RDONLY | O_DIRECT);
		if (fd_ < 0) {
			perror("open direct read");
			return false;
		}

		for (size_t i = 0; i < config_.num_operations; ++i) {
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			if (lseek(fd_, offset, SEEK_SET) < 0) {
				perror("lseek");
				return false;
			}
			if (read(fd_, buffer_, config_.block_size) !=
				(ssize_t)config_.block_size) {
				perror("read");
				return false;
			}
		}
		return true;
	}

	bool run_write() override {
		fd_ = open(config_.filename.c_str(), O_WRONLY | O_DIRECT);
		if (fd_ < 0) {
			perror("open direct write");
			return false;
		}

		memset(buffer_, 'C', config_.block_size);
		for (size_t i = 0; i < config_.num_operations; ++i) {
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			if (lseek(fd_, offset, SEEK_SET) < 0) {
				perror("lseek");
				return false;
			}
			if (write(fd_, buffer_, config_.block_size) !=
				(ssize_t)config_.block_size) {
				perror("write");
				return false;
			}
		}
		return true;
	}

	void cleanup() override {
		if (fd_ >= 0) {
			close(fd_);
			fd_ = -1;
		}
		if (buffer_) {
			free(buffer_);
			buffer_ = nullptr;
		}
	}

	std::string name() const override { return "Direct IO"; }
};

// Linux AIO
class LinuxAIO : public IOMethod {
  private:
	Config config_;
	int fd_ = -1;
	char *buffer_ = nullptr;
	io_context_t ctx_ = 0;

  public:
	bool init(const Config &config) override {
		config_ = config;
		buffer_ = new char[config_.block_size * config_.num_operations];
		if (io_setup(config_.num_operations, &ctx_) < 0) {
			perror("io_setup");
			return false;
		}
		return true;
	}

	bool run_read() override {
		fd_ = open(config_.filename.c_str(), O_RDONLY | O_DIRECT);
		if (fd_ < 0) {
			perror("open aio read");
			return false;
		}

		std::vector<iocb *> iocbs(config_.num_operations);
		std::vector<iocb> cb(config_.num_operations);

		for (size_t i = 0; i < config_.num_operations; ++i) {
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			memset(&cb[i], 0, sizeof(iocb));
			cb[i].aio_fildes = fd_;
			cb[i].aio_lio_opcode = IO_CMD_PREAD;
			cb[i].u.c.buf = buffer_ + i * config_.block_size;
			cb[i].u.c.nbytes = config_.block_size;
			cb[i].u.c.offset = offset;
			iocbs[i] = &cb[i];
		}

		if (io_submit(ctx_, config_.num_operations, iocbs.data()) !=
			(int)config_.num_operations) {
			perror("io_submit");
			return false;
		}

		std::vector<io_event> events(config_.num_operations);
		if (io_getevents(ctx_, config_.num_operations, config_.num_operations,
						 events.data(),
						 nullptr) != (long)config_.num_operations) {
			perror("io_getevents");
			return false;
		}

		return true;
	}

	bool run_write() override {
		fd_ = open(config_.filename.c_str(), O_WRONLY | O_DIRECT);
		if (fd_ < 0) {
			perror("open aio write");
			return false;
		}

		memset(buffer_, 'D', config_.block_size * config_.num_operations);

		std::vector<iocb *> iocbs(config_.num_operations);
		std::vector<iocb> cb(config_.num_operations);

		for (size_t i = 0; i < config_.num_operations; ++i) {
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			memset(&cb[i], 0, sizeof(iocb));
			cb[i].aio_fildes = fd_;
			cb[i].aio_lio_opcode = IO_CMD_PWRITE;
			cb[i].u.c.buf = buffer_ + i * config_.block_size;
			cb[i].u.c.nbytes = config_.block_size;
			cb[i].u.c.offset = offset;
			iocbs[i] = &cb[i];
		}

		if (io_submit(ctx_, config_.num_operations, iocbs.data()) !=
			(int)config_.num_operations) {
			perror("io_submit");
			return false;
		}

		std::vector<io_event> events(config_.num_operations);
		if (io_getevents(ctx_, config_.num_operations, config_.num_operations,
						 events.data(),
						 nullptr) != (long)config_.num_operations) {
			perror("io_getevents");
			return false;
		}

		return true;
	}

	void cleanup() override {
		if (fd_ >= 0) {
			close(fd_);
			fd_ = -1;
		}
		delete[] buffer_;
		buffer_ = nullptr;
		io_destroy(ctx_);
		ctx_ = 0;
	}

	std::string name() const override { return "Linux AIO"; }
};

class IOUring : public IOMethod {
  private:
	Config config_;
	int fd_ = -1;
	char *buffer_ = nullptr;
	struct io_uring ring;
	struct io_uring_params params;

  public:
	bool init(const Config &config) override {
		config_ = config;
		buffer_ = new char[config_.block_size * config_.num_operations];
		memset(&params, 0, sizeof(params));
		params.flags |= IORING_SETUP_SQPOLL;
		params.sq_thread_idle = 1000;
		if (io_uring_queue_init_params(config.num_operations, &ring, &params) !=
			0) {
			perror("io_uring_queue_init_params");
			return false;
		}
		return true;
	}

	bool run_read() override {
		fd_ = open(config_.filename.c_str(), O_RDONLY);
		if (fd_ < 0) {
			perror("open iouring read");
			return false;
		}

		if (io_uring_register_files(&ring, &fd_, 1) < 0) {
			perror("io_uring_register_files");
			return false;
		}

		for (size_t i = 0; i < config_.num_operations; ++i) {
			io_uring_sqe *sqe = io_uring_get_sqe(&ring);
			if (!sqe) {
				perror("io_uring_get_sqe");
				return false;
			}
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			io_uring_prep_read(sqe, fd_, buffer_ + i * config_.block_size,
							   config_.block_size, offset);
		}
		io_uring_submit(&ring);

		struct io_uring_cqe *cqe[config_.num_operations];
		for (size_t i = 0; i < config_.num_operations; ++i) {
			if (io_uring_wait_cqe(&ring, &cqe[i]) < 0) {
				perror("io_uring_wait_cqe");
				return false;
			}
			if (cqe[i]->res < 0) {
				fprintf(stderr, "Read error: %s\n", strerror(-cqe[i]->res));
				return false;
			}
			// io_uring_cqe_seen(&ring, cqe[i]);
		}

		return true;
	}

	bool run_write() override {
		fd_ = open(config_.filename.c_str(), O_WRONLY);
		if (fd_ < 0) {
			perror("open iouring write");
			return false;
		}

		memset(buffer_, 'E', config_.block_size * config_.num_operations);

		for (size_t i = 0; i < config_.num_operations; ++i) {
			io_uring_sqe *sqe = io_uring_get_sqe(&ring);
			if (!sqe) {
				perror("io_uring_get_sqe");
				return false;
			}
			off_t offset = (i * config_.block_size) %
						   (config_.file_size - config_.block_size);
			io_uring_prep_write(sqe, fd_, buffer_ + i * config_.block_size,
								config_.block_size, offset);
		}
		io_uring_submit(&ring);

		struct io_uring_cqe *cqe[config_.num_operations];
		for (size_t i = 0; i < config_.num_operations; ++i) {
			if (io_uring_wait_cqe(&ring, &cqe[i]) < 0) {
				perror("io_uring_wait_cqe");
				return false;
			}
			if (cqe[i]->res < 0) {
				fprintf(stderr, "Write error: %s\n", strerror(-cqe[i]->res));
				return false;
			}
			// io_uring_cqe_seen(&ring, cqe[i]);
		}

		return true;
	}

	void cleanup() override {
		if (fd_ >= 0) {
			close(fd_);
			fd_ = -1;
		}
		delete[] buffer_;
		buffer_ = nullptr;
		io_uring_queue_exit(&ring);
	}

	std::string name() const override { return "Linux IOUring"; }
};

// Benchmark runner
class BenchmarkRunner {
  public:
	static void run(const Config &config) {
		std::vector<std::unique_ptr<IOMethod>> methods;
		methods.emplace_back(std::make_unique<BufferedIO>());
		methods.emplace_back(std::make_unique<DirectIO>());
		methods.emplace_back(std::make_unique<LinuxAIO>());
		methods.emplace_back(std::make_unique<IOUring>());

		std::cout << "Linux IO Benchmark Results\n";
		std::cout << "=========================\n";
		std::cout << "File size: " << config.file_size / (1024 * 1024)
				  << " MB\n";
		std::cout << "Block size: " << config.block_size << " bytes\n";
		std::cout << "Operations: " << config.num_operations << "\n\n";

		if (!Utils::create_test_file(config.filename, config.file_size)) {
			std::cerr << "Failed to create test file\n";
			return;
		}

		for (auto &method : methods) {
			if (!method->init(config)) {
				std::cerr << "Failed to initialize " << method->name() << "\n";
				continue;
			}

			std::cout << method->name() << ":\n";

			double read_time =
				Utils::benchmark([&]() { return method->run_read(); });
			if (read_time > 0) {
				double read_throughput =
					(config.num_operations * config.block_size) /
					(read_time * 1000.0);
				std::cout << "  Read:  " << read_time << " ms ("
						  << read_throughput << " MB/s)\n";
			}

			double write_time =
				Utils::benchmark([&]() { return method->run_write(); });
			if (write_time > 0) {
				double write_throughput =
					(config.num_operations * config.block_size) /
					(write_time * 1000.0);
				std::cout << "  Write: " << write_time << " ms ("
						  << write_throughput << " MB/s)\n";
			}

			method->cleanup();
			std::cout << std::endl;
		}

		Utils::remove_file(config.filename);
	}
};

int main(int argc, char *argv[]) {
	Config config;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--file" && i + 1 < argc) {
			config.filename = argv[++i];
		} else if (arg == "--size" && i + 1 < argc) {
			config.file_size = std::stoull(argv[++i]) * 1024 * 1024;
		} else if (arg == "--block" && i + 1 < argc) {
			config.block_size = std::stoull(argv[++i]);
		} else if (arg == "--ops" && i + 1 < argc) {
			config.num_operations = std::stoull(argv[++i]);
		} else if (arg == "--verbose") {
			config.verbose = true;
		} else if (arg == "--help") {
			std::cout
				<< "Usage: " << argv[0] << " [options]\n"
				<< "Options:\n"
				<< "  --file <filename>    Test file name (default: "
				   "testfile.dat)\n"
				<< "  --size <MB>          File size in MB (default: 1024)\n"
				<< "  --block <bytes>      Block size in bytes (default: "
				   "4096)\n"
				<< "  --ops <count>        Number of operations (default: "
				   "1000)\n"
				<< "  --verbose            Enable verbose output\n"
				<< "  --help               Show this help\n";
			return 0;
		}
	}

	BenchmarkRunner::run(config);
	return 0;
}