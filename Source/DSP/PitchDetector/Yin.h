/*
  ==============================================================================

    Yin.h
    Created: 18 Oct 2023 4:07:20pm
    Author:  sunwei06

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <complex>
#include <map>
#include <tuple>
#include <vector>

#define YIN_THRESHOLD 0.20
#define PYIN_PA 0.01
#define PYIN_N_THRESHOLDS 100
#define PYIN_MIN_THRESHOLD 0.01

using fucking = const float;

namespace Yin {

	struct Yin_Result {
		size_t N;
		std::vector<double> out_real;
		std::vector<double> yin_buffer;
	};

	/// <summary>
	/// Beta distribution params
	/// </summary>
	static const double Beta_Distribution[100] = { 0.012614, 0.022715, 0.030646,
	0.036712, 0.041184, 0.044301, 0.046277, 0.047298, 0.047528, 0.047110,
	0.046171, 0.044817, 0.043144, 0.041231, 0.039147, 0.036950, 0.034690,
	0.032406, 0.030133, 0.027898, 0.025722, 0.023624, 0.021614, 0.019704,
	0.017900, 0.016205, 0.014621, 0.013148, 0.011785, 0.010530, 0.009377,
	0.008324, 0.007366, 0.006497, 0.005712, 0.005005, 0.004372, 0.003806,
	0.003302, 0.002855, 0.002460, 0.002112, 0.001806, 0.001539, 0.001307,
	0.001105, 0.000931, 0.000781, 0.000652, 0.000542, 0.000449, 0.000370,
	0.000303, 0.000247, 0.000201, 0.000162, 0.000130, 0.000104, 0.000082,
	0.000065, 0.000051, 0.000039, 0.000030, 0.000023, 0.000018, 0.000013,
	0.000010, 0.000007, 0.000005, 0.000004, 0.000003, 0.000002, 0.000001,
	0.000001, 0.000001, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
	0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
	0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
	0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000 };

	class Yin_Pitch {

	private:
		std::vector<double> tempContainer;

		Yin_Result* rslt;

		static std::pair<double, double>parabolic_interpolation(const std::vector<double>& array, int x_)
		{
			int x_adjusted;
			double x = (double)x_;

			if (x < 1) {
				x_adjusted = (array[x] <= array[x + 1]) ? x : x + 1;
			}
			else if (x > signed(array.size()) - 1) {
				x_adjusted = (array[x] <= array[x - 1]) ? x : x - 1;
			}
			else {
				double den = array[x + 1] + array[x - 1] - 2 * array[x];
				double delta = array[x - 1] - array[x + 1];
				return (!den) ? std::make_pair(x, array[x])
					: std::make_pair(x + delta / (2 * den),
						array[x] - delta * delta / (8 * den));
			}
			return std::make_pair(x_adjusted, array[x_adjusted]);
		}

		static void acorr_r(fucking* audio_buffer, size_t& size, Yin::Yin_Result*& fuck)
		{
			double r = 0;
			for (size_t tao = 1; tao < size; ++tao) {
				r = 0;
				for (size_t j = 0; j < size; ++j) {
					r += audio_buffer[j] * audio_buffer[j + tao];
				}
				fuck->out_real.push_back(r);
			}
			fuck->N = fuck->out_real.size();
		}

		static void difference(fucking* audio_buffer, size_t& size, Yin::Yin_Result* ya)
		{
			acorr_r(audio_buffer, size, ya);

			for (int tau = 0; tau < ya->N; tau++)
				ya->yin_buffer.push_back(ya->out_real[0] + ya->out_real[1] - 2 * ya->out_real[tau]);
		}

		static void cumulative_mean_normalized_difference(std::vector<double>& yin_buffer)
		{
			double running_sum = 0.0f;

			yin_buffer[0] = 1;

			for (int tau = 1; tau < signed(yin_buffer.size()); tau++) {
				running_sum += yin_buffer[tau];
				yin_buffer[tau] *= tau / running_sum;
			}
		}

		static size_t absolute_threshold(std::vector<double>& yin_buffer)
		{
			size_t size = yin_buffer.size();
			int tau;
			for (tau = 2; tau < size; tau++) {
				if (yin_buffer[tau] < YIN_THRESHOLD) {
					while (tau + 1 < size && yin_buffer[tau + 1] < yin_buffer[tau]) {
						tau++;
					}
					break;
				}
			}
			return (tau == size || yin_buffer[tau] >= YIN_THRESHOLD) ? -1 : tau;
		}

	public:
		double Pitch(fucking* audio_buffer, size_t size, size_t sample_rate) {
			int tau_estimate;
			
			rslt = new Yin_Result;
			
			difference(audio_buffer, size, rslt);

			cumulative_mean_normalized_difference(rslt->yin_buffer);
			tau_estimate = absolute_threshold(rslt->yin_buffer);

			auto ret = (tau_estimate != -1)
				? sample_rate / std::get<0>(parabolic_interpolation(
					rslt->yin_buffer, tau_estimate))
				: -1;
			delete rslt;
			return ret;
		}
	};

}
