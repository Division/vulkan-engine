#pragma once

#include <unordered_set>

template <class T>
class CapsSet {
public:
	typedef uint32_t Bitmask;

	CapsSet() = default;
	CapsSet(const CapsSet &other) = default;
	CapsSet &operator=(const CapsSet &other) = default;

	CapsSet(CapsSet &&other) {
		_bitmask = other._bitmask;
		_maskDirty = other._maskDirty;
		_caps = std::move(other._caps);
		other._bitmask = 0;
		other._maskDirty = false;
	}

	inline const std::unordered_set<Bitmask> caps() const {
		return _caps;
	}

	inline bool hasCap(T cap) const { 
		return _caps.find((Bitmask)cap) != _caps.end();
	};
	
	inline void addCap(T cap) {
		_caps.insert((Bitmask)cap);
		_maskDirty = true;
	}

	inline bool containsCaps(const CapsSet<T> &other) {
		auto bitmask = getBitmask();
		auto otherBitmask = other.getBitmask();
		return bitmask & otherBitmask == otherBitmask;
	}

	inline void removeCap(T cap) {
		_caps.erase((Bitmask)cap);
		_maskDirty = true;
	}

	inline Bitmask getBitmask() const {
		if (_maskDirty) {
			_bitmask = _calculateBitMask();
			_maskDirty = false;
		}
		return _bitmask;
	}

private:
	mutable Bitmask _bitmask = 0; // cached bitmask
	mutable bool _maskDirty = false;
	std::unordered_set<Bitmask> _caps;

private:
	inline Bitmask _calculateBitMask() const {
		Bitmask result = 0;

		for (auto cap : _caps) {
			result |= 1u << (Bitmask)cap;
		}

		return result;
	}
};
