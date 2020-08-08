/*
 * Component.h
 *
 *  Created on: Mar 2, 2019
 *      Author: nick
 */

#ifndef SRC_SYSTEMCOMPONENT_H_
#define SRC_SYSTEMCOMPONENT_H_

class SystemComponent {
public:
  SystemComponent();
  virtual ~SystemComponent();

  virtual void initialize() = 0;
  virtual void start() = 0;
  virtual void destroy() = 0;
  virtual void stop() = 0;

};

#endif /* SRC_SYSTEMCOMPONENT_H_ */
