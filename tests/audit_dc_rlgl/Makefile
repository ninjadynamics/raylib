CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror
N3 := -DRLDC_USE_FINAL_PACKET_FAST_PATH=1
HYPERSOLAR_TRUSTED := -DRLDC_HYPERSOLAR_TRUSTED_N3=1

.PHONY: check clean

check: test_batch_default_f1.exe test_batch_fast.exe test_batch_checked_n3.exe test_batch_threshold.exe test_batch_stats.exe test_batch_native_bench.exe test_batch_disabled.exe test_batch_n3_disabled.exe test_batch_n3_feature_zero.exe test_batch_trusted_n3_feature_zero.exe test_batch_f1_feature_zero.exe test_batch_features_zero.exe test_batch_abi_mismatch.exe test_batch_legacy.exe
	./test_batch_default_f1.exe
	./test_batch_fast.exe
	./test_batch_checked_n3.exe
	./test_batch_threshold.exe
	./test_batch_stats.exe
	./test_batch_native_bench.exe
	./test_batch_disabled.exe
	./test_batch_n3_disabled.exe
	./test_batch_n3_feature_zero.exe
	./test_batch_trusted_n3_feature_zero.exe
	./test_batch_f1_feature_zero.exe
	./test_batch_features_zero.exe
	./test_batch_abi_mismatch.exe
	./test_batch_legacy.exe

test_batch_default_f1.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=0 -o $@ test_batch.c

test_batch_fast.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) $(HYPERSOLAR_TRUSTED) -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=1 -o $@ test_batch.c

test_batch_checked_n3.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=1 -o $@ test_batch.c

test_batch_threshold.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DRLDC_FINAL_PACKET_MIN_VERTICES=4 -DAUDIT_TEST_MIN_THRESHOLD=1 -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=1 -o $@ test_batch.c

test_batch_stats.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) $(HYPERSOLAR_TRUSTED) -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=1 -DRLDC_ENABLE_STATS=1 -o $@ test_batch.c

test_batch_native_bench.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=1 -DGLDC_NATIVE_BENCH=1 -DRLDC_ENABLE_STATS=1 -o $@ test_batch.c

test_batch_disabled.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=0 -DAUDIT_EXPECT_N3=0 -DRLDC_USE_INTERLEAVED_FAST_PATH=0 -o $@ test_batch.c

test_batch_n3_disabled.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=0 -DRLDC_USE_FINAL_PACKET_FAST_PATH=0 -o $@ test_batch.c

test_batch_n3_feature_zero.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=0 -DAUDIT_GLKOS_FINAL_FEATURE_ZERO=1 -o $@ test_batch.c

test_batch_trusted_n3_feature_zero.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=1 -DAUDIT_EXPECT_N3=0 -DAUDIT_GLKOS_TRUSTED_FINAL_FEATURE_ZERO=1 -o $@ test_batch.c

test_batch_f1_feature_zero.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=0 -DAUDIT_EXPECT_N3=1 -DAUDIT_GLKOS_INTERLEAVED_FEATURE_ZERO=1 -o $@ test_batch.c

test_batch_features_zero.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=0 -DAUDIT_EXPECT_N3=0 -DAUDIT_GLKOS_INTERLEAVED_FEATURE_ZERO=1 -DAUDIT_GLKOS_FINAL_FEATURE_ZERO=1 -o $@ test_batch.c

test_batch_abi_mismatch.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. $(N3) -DAUDIT_EXPECT_F1=0 -DAUDIT_EXPECT_N3=0 -DAUDIT_GLKOS_ABI_V1=1 -o $@ test_batch.c

test_batch_legacy.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=0 -DAUDIT_EXPECT_N3=0 -DAUDIT_GLKOS_LEGACY=1 -o $@ test_batch.c

clean:
	$(RM) test_batch_default_f1.exe test_batch_fast.exe test_batch_checked_n3.exe test_batch_threshold.exe test_batch_stats.exe test_batch_native_bench.exe test_batch_disabled.exe test_batch_n3_disabled.exe test_batch_n3_feature_zero.exe test_batch_trusted_n3_feature_zero.exe test_batch_f1_feature_zero.exe test_batch_features_zero.exe test_batch_abi_mismatch.exe test_batch_legacy.exe
