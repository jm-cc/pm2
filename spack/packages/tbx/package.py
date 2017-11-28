from spack import *
import platform

class Tbx(AutotoolsPackage):
    """TBX pm2 toolbox"""
    homepage = "http://pm2.gforge.inria.fr/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/pm2/trunk/tbx')

    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')

    depends_on('pkgconfig', type='build')
    depends_on('autoconf', type='build')
    depends_on('libtool', type='build')
    depends_on('automake', type='build')
    depends_on('hwloc')
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--with-hwloc',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'     in spec else 'disable'),
            "--%s-optimize"      % ('enable' if '+optimize'  in spec else 'disable'),
        ])

        return config_args
    
        
