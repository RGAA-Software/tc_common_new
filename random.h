//
// Created by RGAA on 2023-12-18.
//

#ifndef TC_APPLICATION_RANDOM_H
#define TC_APPLICATION_RANDOM_H

#include <cstdlib>
#include <ctime>

namespace tc
{

    class Random {
    public:

        template<typename T>
        static T RandT(T _min, T _max) {
            T temp;
            if (_min > _max)
            {
                temp = _max;
                _max = _min;
                _min = temp;
            }
            if (_max - _min == 0) {
                return _min;
            }
            return rand() / (double)RAND_MAX * (_max - _min) + _min;
        }

    };

}

#endif //TC_APPLICATION_RANDOM_H
