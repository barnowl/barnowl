-- MySQL dump 10.10
--
-- Host: localhost    Database: zephyr_log
-- ------------------------------------------------------
-- Server version	5.0.26-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `messages`
--

DROP TABLE IF EXISTS `messages`;
CREATE TABLE `messages` (
  `id` int(11) NOT NULL auto_increment,
  `sender` varchar(100) NOT NULL default '',
  `class` varchar(100) NOT NULL default '',
  `instance` varchar(100) NOT NULL default '',
  `opcode` varchar(100) default NULL,
  `sent` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `received` timestamp NULL default NULL,
  `host` varchar(200) NOT NULL default '',
  `zsig` text NOT NULL,
  `body` text,
  PRIMARY KEY  (`id`),
  KEY `index_class` (`class`),
  KEY `index_instance` (`instance`),
  KEY `index_class_instance` (`class`,`instance`),
  KEY `index_class_instance_sender` (`class`,`instance`,`sender`),
  KEY `index_class_sender` (`class`,`sender`),
  KEY `index_sender` (`sender`),
  KEY `index_host` (`host`),
  KEY `index_sent` (`sent`),
  FULLTEXT KEY `index_body` (`body`)
) ENGINE=MyISAM AUTO_INCREMENT=1517065 DEFAULT CHARSET=utf8;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2007-02-06 21:39:09
