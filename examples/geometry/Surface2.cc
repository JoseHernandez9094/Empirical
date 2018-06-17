//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2018.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  Some examples code for using emp::Random

#include <iostream>
#include "geometry/Point2D.h"
#include "geometry/Surface2.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/string_utils.h"

struct TestBody {
  size_t id = 0;
};

//   std::string AsString() const {
//     size_t x = (size_t) (center.GetX() + 0.5);
//     size_t y = (size_t) (center.GetY() + 0.5);
//     return emp::to_string("(", x, ",", y, "):", (size_t) radius);
//   }


// void PrintOverlap(const TestBody & body1, const TestBody & body2) {
//   std::cout << "Overlap " << body1.id << " and " << body2.id << ": "
//             << body1.AsString() << " and " << body2.AsString()
//             << "  dist=" << body1.GetCenter().Distance(body2.GetCenter())
//             << std::endl;
// }

void PrintOverlap(const TestBody & body1, const TestBody & body2) {
  std::cout << "Overlap " << body1.id << " and " << body2.id << ".\n";
}


int main()
{
  emp::Random random;
  emp::Surface<TestBody> surface({1000, 1000});
  emp::vector<TestBody> bodies(20);

  for (auto & body : bodies) {
    body.id = surface.AddBody(&body,
			      {random.GetDouble(1000.0), random.GetDouble(1000.0)},
			      random.GetDouble(10.0, 100.0));
  }

  std::cout << "---- surface results ----" << std::endl;
  surface.FindOverlaps(PrintOverlap);

//   std::cout << "\n---- brute force ----" << std::endl;
//   for (size_t id1 = 1; id1 < bodies.size(); id1++) {
//     auto & body1 = bodies[id1];
//     for (size_t id2 = 0; id2 < id1; id2++) {
//       auto & body2 = bodies[id2];
//       double radius_sum = body1.GetRadius() + body2.GetRadius();
//       if (body1.GetCenter().Distance(body2.GetCenter()) < radius_sum) {
//         PrintOverlap(body1, body2);
//       }
//     }
//   }

  std::cout << std::endl;
}